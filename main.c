
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <git2.h>

/* ============================================
 * CROSS-PATFORM ABSTRACTIONS
 * ============================================ */

#ifdef _WIN32
    /* ncurses for Windows via MSYS2 */
    #include <ncursesw/ncurses.h>
    #include <windows.h>
    #include <direct.h>
    #include <io.h>

    /* Windows-specific function wrappers */
    #define MKDIR(path) _mkdir(path)
    #define DIR_EXISTS(path) (_access(path, 0) == 0)

    /* Windows uses nul for /dev/null */
    #define DEV_NULL "> nul 2>&1"
    #define PATH_SEP '\\'

    /* ASCII control key definitions */
    #define CTRL_S 19
    #define CTRL_X 24

#else
    /* ncurses for Unix (Linux/macOS) */
    #include <ncurses.h>
    #include <unistd.h>

    /* Unix-specific function wrappers */
    #define MKDIR(path) mkdir(path, 0700)
    #define DIR_EXISTS(path) (access(path, F_OK) == 0)

    /* Unix uses /dev/null */
    #define DEV_NULL "> /dev/null 2>&1"
    #define PATH_SEP '/'

    /* ASCII control key definitions */
    #define CTRL_S 19
    #define CTRL_X 24
#endif

/* ============================================
 * EDITOR CONSTANTS
 * ============================================ */

#define MAX_LINES 1000       
#define MAX_COLS 256         

/* ============================================
 * GLOBAL STATE
 * ============================================ */

/* The main text buffer: a 2D array of characters */
char text_buffer[MAX_LINES][MAX_COLS] = {0};

/* Cursor position tracking */
int cursor_x = 0;
int cursor_y = 0;

/* ============================================
 * HELPER FUNCTIONS
 * ============================================ */


char* get_home_dir() {
    char* home = getenv("HOME");
    
#ifdef _WIN32
    /* Fallback to USERPROFILE if HOME isn't set on Windows */
    if (!home) {
        home = getenv("USERPROFILE");
    }
#endif

    return home;
}


char* get_github_username() {
    static char username[256];
    FILE* pipe = popen("gh api user --jq '.login'", "r");
    if (pipe) {
        if (fgets(username, sizeof(username), pipe) != NULL) {
            username[strcspn(username, "\n")] = 0; /* Remove newline */
            pclose(pipe);
            return username;
        }
        pclose(pipe);
    }
    return "unknown"; /* Fallback */
}

void draw_ui(int max_y, int max_x) {
    
    attron(A_REVERSE);  
    mvprintw(0, 0, "%-*s", max_x, " Syncpad - notes.txt (Autosync Active)");
    attroff(A_REVERSE); 

    attron(A_REVERSE);
    mvprintw(max_y - 1, 0, "%-*s", max_x, " ^S Save & Sync  |  ^X Exit");
    attroff(A_REVERSE);
}

int save_file(const char* filepath) {

    FILE *debug = fopen("/tmp/syncpad_debug.log", "w");
    if (debug) {
        fprintf(debug, "=== save_file DEBUG LOG ===\n");
        fprintf(debug, "Called with filepath: '%s'\n", filepath);
        fprintf(debug, "cursor_y = %d, cursor_x = %d\n", cursor_y, cursor_x);
        fprintf(debug, "text_buffer[0] = '%s'\n", text_buffer[0]);
        fprintf(debug, "text_buffer[1] = '%s'\n", text_buffer[1]);
        fclose(debug);
    }
    
    FILE *fp = fopen(filepath, "w");
    if (!fp) {
        
        FILE *err_log = fopen("/tmp/syncpad_error.log", "w");
        if (err_log) {
            fprintf(err_log, "Failed to open filepath for writing: '%s'\n", filepath);
            fclose(err_log);
        }
        return -1;
    }
    
    for (int i = 0; i <= cursor_y; i++) {
        fprintf(fp, "%s\n", text_buffer[i]);
    }
    fclose(fp);
    
    /* DEBUG: Confirm file was written */
    debug = fopen("/tmp/syncpad_debug.log", "a");
    if (debug) {
        fprintf(debug, "File written successfully!\n");
        fclose(debug);
    }
    
    return 0;
}

void verify_git_config(const char* sync_dir) {
    char cmd[1024];
    
    /* Check user.name */
    snprintf(cmd, sizeof(cmd), "cd \"%s\" && git config user.name", sync_dir);
    if (system(cmd) != 0) {
        printf("\nGit 'user.name' not set. Syncpad needs this for commits.\n");
        printf("Enter your name: ");
        char name[256];
        if (fgets(name, sizeof(name), stdin)) {
            name[strcspn(name, "\n")] = 0;
            snprintf(cmd, sizeof(cmd), "cd \"%s\" && git config user.name \"%s\"", sync_dir, name);
            system(cmd);
        }
    }

    /* Check user.email */
    snprintf(cmd, sizeof(cmd), "cd \"%s\" && git config user.email", sync_dir);
    if (system(cmd) != 0) {
        printf("\nGit 'user.email' not set. Syncpad needs this for commits.\n");
        printf("Enter your email: ");
        char email[256];
        if (fgets(email, sizeof(email), stdin)) {
            email[strcspn(email, "\n")] = 0;
            snprintf(cmd, sizeof(cmd), "cd \"%s\" && git config user.email \"%s\"", sync_dir, email);
            system(cmd);
        }
    }
}

int verify_git_auth(const char* sync_dir) {
    char cmd[1024];
    printf("Verifying GitHub authentication...\n");
    
    /* Create a temporary test file and try to push it */
    char test_file[512];
    snprintf(test_file, sizeof(test_file), "%s%c.auth_test", sync_dir, PATH_SEP);
    
    /* Create empty test file */
    FILE *fp = fopen(test_file, "w");
    if (fp) {
        fclose(fp);
    } else {
        return 0;
    }
    
    /* Try to add, commit, and push */
    snprintf(cmd, sizeof(cmd), 
        "cd \"%s\" && git add .auth_test && "
        "git commit -m \"Auth test\" %s && git push origin main %s && "
        "git rm .auth_test && git commit -m \"Cleanup auth test\" %s && git push origin main %s",
        sync_dir, DEV_NULL, DEV_NULL, DEV_NULL, DEV_NULL);
    
    if (system(cmd) == 0) {
        return 1;
    }
    git_libgit2_shutdown();
    return 0;
}

int sync_to_github_in_background(const char* sync_dir) {
    git_repository *repo = NULL;
    git_index *index = NULL;
    git_oid tree_id, commit_id, parent_id;
    git_tree *tree = NULL;
    git_commit *parent = NULL;
    git_signature *me = NULL;
    git_remote *remote = NULL;
    git_push_options options = GIT_PUSH_OPTIONS_INIT;
    git_status_list *status = NULL;
    git_status_options statusopt = GIT_STATUS_OPTIONS_INIT;
    int error = 0;
    int result = 0;

    FILE *git_debug = fopen("/tmp/syncpad_libgit2_debug.log", "w");
    if (git_debug) {
        fprintf(git_debug, "=== libgit2 sync DEBUG LOG ===\n");
        fprintf(git_debug, "sync_dir: '%s'\n", sync_dir);
        fclose(git_debug);
    }

    /* 1. Open the repository */
    if ((error = git_repository_open(&repo, sync_dir)) != 0) {
        git_debug = fopen("/tmp/syncpad_libgit2_debug.log", "a");
        if (git_debug) {
            fprintf(git_debug, "ERROR: Failed to open repo, code: %d\n", error);
            fclose(git_debug);
        }
        return error;
    }
    
    git_debug = fopen("/tmp/syncpad_libgit2_debug.log", "a");
    if (git_debug) {
        fprintf(git_debug, "Repo opened successfully\n");
        fclose(git_debug);
    }

    /* 2. Check if notes.txt changed */
    statusopt.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    statusopt.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX | GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;
    
    if (git_status_list_new(&status, repo, &statusopt) == 0) {
        size_t count = git_status_list_entrycount(status);
        git_debug = fopen("/tmp/syncpad_libgit2_debug.log", "a");
        if (git_debug) {
            fprintf(git_debug, "Status entry count: %zu\n", count);
            fclose(git_debug);
        }
        
        if (count > 0) {
            /* 3. Add notes.txt to the staging area (index) */
            if (git_repository_index(&index, repo) == 0) {
                error = git_index_add_bypath(index, "notes.txt");
                git_debug = fopen("/tmp/syncpad_libgit2_debug.log", "a");
                if (git_debug) {
                    fprintf(git_debug, "git_index_add_bypath result: %d\n", error);
                    fclose(git_debug);
                }
                
                git_index_write(index);
                git_index_write_tree(&tree_id, index);
                git_index_free(index);
            }

            /* 4. Create the commit */
            if (git_tree_lookup(&tree, repo, &tree_id) == 0) {
                /* Find HEAD*/
                if (git_reference_name_to_id(&parent_id, repo, "HEAD") == 0 &&
                    git_commit_lookup(&parent, repo, &parent_id) == 0) {
                    
                    /* Create a signature */
                    if (git_signature_default(&me, repo) != 0) {
                        git_signature_now(&me, "Syncpad Bot", "bot@syncpad.local");
                    }

                    /* Create the commit */
                    error = git_commit_create_v(
                        &commit_id, repo, "HEAD", me, me,
                        NULL, "Auto-sync from Syncpad", tree, 1, parent
                    );
                    
                    git_debug = fopen("/tmp/syncpad_libgit2_debug.log", "a");
                    if (git_debug) {
                        fprintf(git_debug, "git_commit_create result: %d\n", error);
                        fclose(git_debug);
                    }
                    
                    if (error != 0) result = error;
                    
                    git_signature_free(me);
                    git_commit_free(parent);
                }
                git_tree_free(tree);
            }
        } else {
            git_debug = fopen("/tmp/syncpad_libgit2_debug.log", "a");
            if (git_debug) {
                fprintf(git_debug, "No changes detected, skipping commit\n");
                fclose(git_debug);
            }
        }
    }
    git_status_list_free(status);

    /* 5. Cleanup libgit2 resources befo pushing*/
    git_repository_free(repo);

    /* 6. Push to origin using system Git */
    char push_cmd[2048];
    snprintf(push_cmd, sizeof(push_cmd), "cd \"%s\" && git push origin main -q > /dev/null 2>&1", sync_dir);
    
    if (system(push_cmd) != 0) {
        result = -1; 
        git_debug = fopen("/tmp/syncpad_libgit2_debug.log", "a");
        if (git_debug) {
            fprintf(git_debug, "System git push failed\n");
            fclose(git_debug);
        }
    } else {
        git_debug = fopen("/tmp/syncpad_libgit2_debug.log", "a");
        if (git_debug) {
            fprintf(git_debug, "System git push succeeded!\n");
            fclose(git_debug);
        }
    }

    return result;
}

/* ============================================
 * MAIN APPLICATION
 * ============================================ */

int main() {
    git_libgit2_init();

    char sync_dir[1024];
    char notes_file[1024];
    char cmd_buffer[2048];
    
    snprintf(sync_dir, sizeof(sync_dir), "%s%c.syncpad", get_home_dir(), PATH_SEP);
    
    snprintf(notes_file, sizeof(notes_file), "%s%cnotes.txt", sync_dir, PATH_SEP);

    if (!DIR_EXISTS(sync_dir)) {
        printf("Welcome to Syncpad!\n");
        int setup_done = 0;

        if (system("gh --version " DEV_NULL) == 0) {
            printf("GitHub CLI detected. Would you like to automatically create\n");
            printf("a private 'syncpad-notes' repository? (y/n): ");
            
            char choice[10];
            if (fgets(choice, sizeof(choice), stdin) && (choice[0] == 'y' || choice[0] == 'Y')) {
                printf("Attempting to clone or create 'syncpad-notes'...\n");
                
                snprintf(cmd_buffer, sizeof(cmd_buffer), "gh repo clone \"syncpad-notes\" \"%s\" " DEV_NULL, sync_dir);
                if (system(cmd_buffer) == 0) {
                    setup_done = 1;
                    printf("Successfully cloned existing repository!\n");
                } else {
                    /* If clone fails try to create repo */
                    printf("Repository not found. Creating 'syncpad-notes'...\n");
                    int create_result = system("gh repo create syncpad-notes --private --add-readme");
                    if (create_result == 0) {
                        printf("Created and now cloning...\n");
                        snprintf(cmd_buffer, sizeof(cmd_buffer), "gh repo clone \"syncpad-notes\" \"%s\" " DEV_NULL, sync_dir);
                        if (system(cmd_buffer) == 0) {
                            setup_done = 1;
                        } else {
                            printf("Clone after creation failed. Falling back to manual setup.\n");
                        }
                    } else {
                        printf("Could not create repository either. Falling back to manual setup.\n");
                    }
                }
            }
        }

        if (!setup_done) {
            printf("Enter your private GitHub SSH repo URL: ");
            
            char url[512];
            if (fgets(url, sizeof(url), stdin) != NULL) {
                url[strcspn(url, "\n")] = 0;
                
                printf("Cloning repository (please ensure SSH keys are set up)...\n");
                
                snprintf(cmd_buffer, sizeof(cmd_buffer), "git clone \"%s\" \"%s\"", url, sync_dir);
                if (system(cmd_buffer) != 0) {
                    return 1;
                }
            } else {
                return 1;
            }
        }

        verify_git_config(sync_dir);
        if (!verify_git_auth(sync_dir)) {
            printf("\n⚠ GitHub Authentication failed!\n");
            printf("Syncpad couldn't push to your repository. This usually happens if:\n");
            printf("1. Your SSH keys aren't added to GitHub\n");
            printf("2. Your SSH agent isn't running\n");
            printf("3. The repository is incorrect\n\n");
            printf("Please fix this and restart Syncpad. Press Enter to exit...");
            getchar();
            return 1;
        }

    } else {
        /* Sync any changes from other computers before UI loads */
        printf("Syncing notes from the cloud...\n");
        
        snprintf(cmd_buffer, sizeof(cmd_buffer), "cd \"%s\" && git pull origin main -q %s", sync_dir, DEV_NULL);
        system(cmd_buffer);
    }

    FILE *fp = fopen(notes_file, "r");
    if (fp) {
        cursor_y = 0;
        while (fgets(text_buffer[cursor_y], MAX_COLS, fp) && cursor_y < MAX_LINES - 1) {
            text_buffer[cursor_y][strcspn(text_buffer[cursor_y], "\n")] = 0;
            cursor_y++;
        }
        fclose(fp);
        
        cursor_y = (cursor_y > 0) ? cursor_y - 1 : 0;
        cursor_x = strlen(text_buffer[cursor_y]);
    }

    /* Verify Git is available before entering UI */
    if (system("git --version " DEV_NULL) != 0) {
        printf("\nERROR: Git is not installed or not in PATH.\n");
        printf("Syncpad requires Git for synchronization.\n");
        printf("Please install Git and try again.\n");
        printf("Visit: https://git-scm.com/downloads\n\n");
        return 1;
    }

#ifndef _WIN32
    /* Crucial fix: Disable software flow control (XON/XOFF).
     * By default, Unix terminals intercept Ctrl+S to suspend output.
     * We disable this so Ctrl+S reaches our ncurses getch() function. */
    system("stty -ixon");
#endif

    initscr();             
    cbreak();              
    noecho();             
    keypad(stdscr, TRUE);  

    /* Get terminal dimensions */
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    while (1) {
        /* CLS */
        clear();
        
        /* Draw the header and footer */
        draw_ui(max_y, max_x);

        /* Render all lines in the text buffer */
        for (int i = 0; i <= cursor_y && i < max_y - 2; i++) {
            mvprintw(i + 1, 0, "%s", text_buffer[i]);
        }

        /* Place the cursor at current position */
        move(cursor_y + 1, cursor_x);
        refresh();

        int ch = getch();
        
        if (ch == CTRL_X) {
            /* ^X: Exit the application */
            break;
        } 
        else if (ch == CTRL_S) {
            /* ^S: Save & Sync */
            
            /* Show "Syncing" message in status bar */
            attron(A_REVERSE);
            mvprintw(max_y - 1, 0, "%-*s", max_x, " Saving locally...    ");
            attroff(A_REVERSE);
            refresh();

            /* Step A: Save buffer to local file */
            if (save_file(notes_file) != 0) {
                attron(A_REVERSE);
                mvprintw(max_y - 1, 0, "%-*s", max_x, " ERROR: Failed to write file to disk! ");
                attroff(A_REVERSE);
                refresh();
                napms(2000);
                continue; 
            }
            
            attron(A_REVERSE);
            mvprintw(max_y - 1, 0, "%-*s", max_x, " Syncing to GitHub... ");
            attroff(A_REVERSE);
            refresh();
            
            /* Step B: Sync using libgit2 */
            int sync_status = sync_to_github_in_background(sync_dir);
            
            /* Show confirmation or error */
            attron(A_REVERSE);
            if (sync_status == 0) {
                mvprintw(max_y - 1, 0, "%-*s", max_x, " Saved & Synced! ");
            } else {
                /* Log the error */
                FILE *err_log = fopen("/tmp/syncpad_git_error.log", "w");
                if (err_log) {
                    fprintf(err_log, "Git sync failed with code: %d\n", sync_status);
                    fclose(err_log);
                }
                mvprintw(max_y - 1, 0, "%-*s", max_x, " Saved locally (Git Sync Failed - Check /tmp/syncpad_git_error.log) ");
            }
            attroff(A_REVERSE);
            refresh();
            napms(1500);  
        }
        
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') { 
            /* Backspace */
            if (cursor_x > 0) {
                cursor_x--;
                text_buffer[cursor_y][cursor_x] = '\0';
            } else if (cursor_y > 0) {
                cursor_y--;
                cursor_x = strlen(text_buffer[cursor_y]);
            }
        }
        else if (ch == '\n' || ch == '\r') { 
            /* Enter */
            if (cursor_y < MAX_LINES - 1) {
                cursor_y++;
                cursor_x = 0;
            }
        }
        else if (ch >= 32 && ch <= 126) { 
            /* Add to buffer */
            if (cursor_x < MAX_COLS - 1) {
                text_buffer[cursor_y][cursor_x] = (char)ch;
                text_buffer[cursor_y][cursor_x + 1] = '\0';
                cursor_x++;
            }
        }
    }

    endwin();  
    
#ifndef _WIN32
    system("stty ixon");
#endif
    
    git_libgit2_shutdown();
    return 0;
}
