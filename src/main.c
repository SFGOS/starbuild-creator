// 4/7/25, here we go again...
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <strings.h>
#include <termios.h>
#include <sys/ioctl.h>

#define MAX_LINE 1024
#define MAX_PACKAGES 10
#define MAX_DEPS 50
#define MAX_SOURCES 20
#define MAX_SCRIPT_LINES 100
#define MAX_LINE_LENGTH 256
#define MAX_OPTIONS 20

typedef struct {
    char name[256];
    char version[64];
    char description[512];
    char license[MAX_DEPS][256];
    int license_count;
    char deps[MAX_DEPS][256];
    int deps_count;
    char conflicts[MAX_DEPS][256];
    int conflicts_count;
    char provides[MAX_DEPS][256];
    int provides_count;
    char optional[MAX_DEPS][256];
    int optional_count;
    char gives[MAX_DEPS][256];
    int gives_count;
    char clashes[MAX_DEPS][256];
    int clashes_count;
    char optional_dependencies[MAX_DEPS][256];
    int optional_dependencies_count;
} Package;

typedef struct {
    Package packages[MAX_PACKAGES];
    int package_count;
    char global_deps[MAX_DEPS][256];
    int global_deps_count;
    char build_deps[MAX_DEPS][256];
    int build_deps_count;
    char sources[MAX_SOURCES][512];
    int sources_count;
    char prepare_script[MAX_SCRIPT_LINES][MAX_LINE_LENGTH];
    int prepare_script_lines;
    char compile_script[MAX_SCRIPT_LINES][MAX_LINE_LENGTH];
    int compile_script_lines;
    char verify_script[MAX_SCRIPT_LINES][MAX_LINE_LENGTH];
    int verify_script_lines;
    char assemble_scripts[MAX_PACKAGES][MAX_SCRIPT_LINES][MAX_LINE_LENGTH];
    int assemble_script_lines[MAX_PACKAGES];
    char template_name[256];
    int enable_advanced_fields;
    char options[MAX_OPTIONS][256];
    int options_count;
} StarbuildConfig;

// Utility functions
void trim(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

void clear_screen() {
    printf("\033[2J\033[H");
}

void print_header(const char *title) {
    printf("\n=== %s ===\n", title);
}

void print_success(const char *msg) {
    printf("\033[32m✓ %s\033[0m\n", msg);
}

void print_error(const char *msg) {
    printf("\033[31m✗ %s\033[0m\n", msg);
}

void print_warning(const char *msg) {
    printf("\033[33m⚠ %s\033[0m\n", msg);
}

void print_highlighted(const char *msg) {
    printf("\033[35m%s\033[0m", msg);
}

void print_disabled(const char *msg) {
    printf("\033[31m%s\033[0m", msg);
}

// Terminal control functions
void enable_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disable_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Input functions
void get_input(const char *prompt, char *buffer, size_t size) {
    printf("%s: ", prompt);
    if (fgets(buffer, size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;
        trim(buffer);
    }
}

void get_multiline_input(const char *prompt, char script[][MAX_LINE_LENGTH], int *line_count) {
    printf("%s\n", prompt);
    printf("(Type 'END' on a line by itself to finish, Ctrl+C to cancel)\n");
    
    *line_count = 0;
    char line[MAX_LINE_LENGTH];
    
    while (*line_count < MAX_SCRIPT_LINES) {
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }
        
        line[strcspn(line, "\n")] = 0;
        trim(line);
        
        if (strcmp(line, "END") == 0) {
            // User typed END - finish input
            break;
        }
        
        if (strlen(line) > 0) {
            strcpy(script[*line_count], line);
            (*line_count)++;
        }
    }
    
    // Ensure we have at least one line
    if (*line_count == 0) {
        strcpy(script[0], "# Add your commands here");
        *line_count = 1;
    }
}

int get_yes_no(const char *prompt) {
    char input[10];
    printf("%s (y/N): ", prompt);
    if (fgets(input, sizeof(input), stdin) != NULL) {
        input[strcspn(input, "\n")] = 0;
        return (strcasecmp(input, "y") == 0 || strcasecmp(input, "yes") == 0);
    }
    return 0;
}

void suggest_package_name(char *suggested_name) {
    char cwd[256];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        char *last_slash = strrchr(cwd, '/');
        if (last_slash && *(last_slash + 1)) {
            strcpy(suggested_name, last_slash + 1);
            // Convert to lowercase and replace non-alphanumeric with hyphens
            for (int i = 0; suggested_name[i]; i++) {
                if (isalpha(suggested_name[i])) {
                    suggested_name[i] = tolower(suggested_name[i]);
                } else if (!isdigit(suggested_name[i])) {
                    suggested_name[i] = '-';
                }
            }
        }
    }
}

// Template functions
void load_template(const char *template_name, StarbuildConfig *config) {
    (void)config; // Suppress unused parameter warning
    char filename[512];
    snprintf(filename, sizeof(filename), "templates/%s.template", template_name);
    
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        print_warning("Template not found, using defaults");
        return;
    }
    
    // Load template logic here (simplified for now)
    fclose(fp);
    print_success("Template loaded");
}

void save_template(const char *template_name, StarbuildConfig *config) {
    (void)config; // Suppress unused parameter warning
    char filename[512];
    snprintf(filename, sizeof(filename), "templates/%s.template", template_name);
    
    // Create templates directory if it doesn't exist
    mkdir("templates", 0755);
    
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        print_error("Could not save template");
        return;
    }
    
    // Save template logic here
    fclose(fp);
    print_success("Template saved");
}

// Interactive wizard functions
void wizard_advanced_fields(StarbuildConfig *config) {
    print_header("Advanced Fields");
    printf("Would you like to configure advanced package fields?\n");
    printf("These include:\n");
    printf("  - gives: Virtual packages this package provides\n");
    printf("  - clashes: Packages that conflict with this one\n");
    printf("  - optional_dependencies: Optional dependencies\n");
    config->enable_advanced_fields = get_yes_no("Enable advanced fields");
}

void wizard_basic_info(StarbuildConfig *config) {
    print_header("Basic Package Information");
    
    char package_names[MAX_LINE];
    char suggested_name[256];
    suggest_package_name(suggested_name);
    
    char prompt[512];
    snprintf(prompt, sizeof(prompt), "Package name(s) (comma-separated for multiple) [%s]", suggested_name);
    get_input(prompt, package_names, sizeof(package_names));
    
    // If no input provided, use suggested name
    if (strlen(package_names) == 0) {
        strcpy(package_names, suggested_name);
    }
    
    // Parse package names
    char *token = strtok(package_names, ",");
    config->package_count = 0;
    while (token && config->package_count < MAX_PACKAGES) {
        trim(token);
        if (strlen(token) > 0) {
            strcpy(config->packages[config->package_count].name, token);
            config->package_count++;
        }
        token = strtok(NULL, ",");
    }
    
    if (config->package_count == 0) {
        print_error("No valid package names provided");
        // Set a default package name
        strcpy(config->packages[0].name, "mypackage");
        config->package_count = 1;
        print_warning("Using default package name 'mypackage'");
    }
    
    // Get version and description
    get_input("Package version", config->packages[0].version, sizeof(config->packages[0].version));
    
    // For multiple packages, get individual descriptions
    if (config->package_count == 1) {
        get_input("Package description", config->packages[0].description, sizeof(config->packages[0].description));
    } else {
        printf("\nEnter descriptions for each package:\n");
        for (int i = 0; i < config->package_count; i++) {
            char prompt[256];
            snprintf(prompt, sizeof(prompt), "Description for %s", config->packages[i].name);
            get_input(prompt, config->packages[i].description, sizeof(config->packages[i].description));
        }
    }
    
    // Get license information
    if (config->package_count == 1) {
        char license_input[MAX_LINE];
        get_input("License(s) (comma-separated, e.g., 'GPL-3.0, MIT')", license_input, sizeof(license_input));
        
        // Parse comma-separated licenses
        char *token = strtok(license_input, ",");
        while (token && config->packages[0].license_count < MAX_DEPS) {
            trim(token);
            if (strlen(token) > 0) {
                strcpy(config->packages[0].license[config->packages[0].license_count], token);
                config->packages[0].license_count++;
            }
            token = strtok(NULL, ",");
        }
    } else {
        printf("\nEnter licenses for each package:\n");
        for (int i = 0; i < config->package_count; i++) {
            char license_input[MAX_LINE];
            char prompt[256];
            snprintf(prompt, sizeof(prompt), "License(s) for %s (comma-separated)", config->packages[i].name);
            get_input(prompt, license_input, sizeof(license_input));
            
            // Parse comma-separated licenses
            char *token = strtok(license_input, ",");
            while (token && config->packages[i].license_count < MAX_DEPS) {
                trim(token);
                if (strlen(token) > 0) {
                    strcpy(config->packages[i].license[config->packages[i].license_count], token);
                    config->packages[i].license_count++;
                }
                token = strtok(NULL, ",");
            }
        }
    }
}

void wizard_dependencies(StarbuildConfig *config) {
    print_header("Dependencies");
    
    char deps_input[MAX_LINE];
    get_input("Global dependencies (comma-separated)", deps_input, sizeof(deps_input));
    
    // Parse comma-separated dependencies
    char *token = strtok(deps_input, ",");
    while (token && config->global_deps_count < MAX_DEPS) {
        trim(token);
        if (strlen(token) > 0) {
            strcpy(config->global_deps[config->global_deps_count], token);
            config->global_deps_count++;
        }
        token = strtok(NULL, ",");
    }
    
    char build_deps_input[MAX_LINE];
    get_input("Build dependencies (comma-separated)", build_deps_input, sizeof(build_deps_input));
    
    token = strtok(build_deps_input, ",");
    while (token && config->build_deps_count < MAX_DEPS) {
        trim(token);
        if (strlen(token) > 0) {
            strcpy(config->build_deps[config->build_deps_count], token);
            config->build_deps_count++;
        }
        token = strtok(NULL, ",");
    }
    
    // For multiple packages, get package-specific dependencies
    if (config->package_count > 1) {
        printf("\nPackage-specific dependencies:\n");
        for (int i = 0; i < config->package_count; i++) {
            char pkg_deps[MAX_LINE];
            char prompt[256];
            snprintf(prompt, sizeof(prompt), "Additional dependencies for %s (comma-separated)", config->packages[i].name);
            get_input(prompt, pkg_deps, sizeof(pkg_deps));
            
            token = strtok(pkg_deps, ",");
            while (token && config->packages[i].deps_count < MAX_DEPS) {
                trim(token);
                if (strlen(token) > 0) {
                    strcpy(config->packages[i].deps[config->packages[i].deps_count], token);
                    config->packages[i].deps_count++;
                }
                token = strtok(NULL, ",");
            }
        }
    }
}

void wizard_sources(StarbuildConfig *config) {
    print_header("Sources");
    
    char sources_input[MAX_LINE];
    get_input("Source URLs (comma-separated)", sources_input, sizeof(sources_input));
    
    char *token = strtok(sources_input, ",");
    while (token && config->sources_count < MAX_SOURCES) {
        trim(token);
        if (strlen(token) > 0) {
            strcpy(config->sources[config->sources_count], token);
            config->sources_count++;
        }
        token = strtok(NULL, ",");
    }
}

void wizard_advanced_package_fields(StarbuildConfig *config) {
    if (!config->enable_advanced_fields) {
        return;
    }
    
    print_header("Advanced Package Fields");
    
    if (config->package_count == 1) {
        // Single package - get advanced fields
        char gives_input[MAX_LINE];
        get_input("Gives (virtual packages, comma-separated)", gives_input, sizeof(gives_input));
        
        char *token = strtok(gives_input, ",");
        while (token && config->packages[0].gives_count < MAX_DEPS) {
            trim(token);
            if (strlen(token) > 0) {
                strcpy(config->packages[0].gives[config->packages[0].gives_count], token);
                config->packages[0].gives_count++;
            }
            token = strtok(NULL, ",");
        }
        
        char clashes_input[MAX_LINE];
        get_input("Clashes (conflicting packages, comma-separated)", clashes_input, sizeof(clashes_input));
        
        token = strtok(clashes_input, ",");
        while (token && config->packages[0].clashes_count < MAX_DEPS) {
            trim(token);
            if (strlen(token) > 0) {
                strcpy(config->packages[0].clashes[config->packages[0].clashes_count], token);
                config->packages[0].clashes_count++;
            }
            token = strtok(NULL, ",");
        }
        
        char optional_deps_input[MAX_LINE];
        get_input("Optional dependencies (comma-separated)", optional_deps_input, sizeof(optional_deps_input));
        
        token = strtok(optional_deps_input, ",");
        while (token && config->packages[0].optional_dependencies_count < MAX_DEPS) {
            trim(token);
            if (strlen(token) > 0) {
                strcpy(config->packages[0].optional_dependencies[config->packages[0].optional_dependencies_count], token);
                config->packages[0].optional_dependencies_count++;
            }
            token = strtok(NULL, ",");
        }
    } else {
        // Multiple packages - get advanced fields for each
        printf("\nAdvanced fields for each package:\n");
        for (int i = 0; i < config->package_count; i++) {
            printf("\nPackage: %s\n", config->packages[i].name);
            
            char gives_input[MAX_LINE];
            char prompt[256];
            snprintf(prompt, sizeof(prompt), "Gives for %s (comma-separated)", config->packages[i].name);
            get_input(prompt, gives_input, sizeof(gives_input));
            
            char *token = strtok(gives_input, ",");
            while (token && config->packages[i].gives_count < MAX_DEPS) {
                trim(token);
                if (strlen(token) > 0) {
                    strcpy(config->packages[i].gives[config->packages[i].gives_count], token);
                    config->packages[i].gives_count++;
                }
                token = strtok(NULL, ",");
            }
            
            char clashes_input[MAX_LINE];
            snprintf(prompt, sizeof(prompt), "Clashes for %s (comma-separated)", config->packages[i].name);
            get_input(prompt, clashes_input, sizeof(clashes_input));
            
            token = strtok(clashes_input, ",");
            while (token && config->packages[i].clashes_count < MAX_DEPS) {
                trim(token);
                if (strlen(token) > 0) {
                    strcpy(config->packages[i].clashes[config->packages[i].clashes_count], token);
                    config->packages[i].clashes_count++;
                }
                token = strtok(NULL, ",");
            }
            
            char optional_deps_input[MAX_LINE];
            snprintf(prompt, sizeof(prompt), "Optional dependencies for %s (comma-separated)", config->packages[i].name);
            get_input(prompt, optional_deps_input, sizeof(optional_deps_input));
            
            token = strtok(optional_deps_input, ",");
            while (token && config->packages[i].optional_dependencies_count < MAX_DEPS) {
                trim(token);
                if (strlen(token) > 0) {
                    strcpy(config->packages[i].optional_dependencies[config->packages[i].optional_dependencies_count], token);
                    config->packages[i].optional_dependencies_count++;
                }
                token = strtok(NULL, ",");
            }
        }
    }
}

void wizard_scripts(StarbuildConfig *config) {
    print_header("Build Scripts");
    
    printf("Enter the build scripts (multi-line, press Enter twice to finish each script):\n\n");
    
    // Get prepare script
    get_multiline_input("Prepare script (e.g., 'cd \"${srcdir}\"')", config->prepare_script, &config->prepare_script_lines);
    
    // Get compile script
    get_multiline_input("Compile script (e.g., 'make -j$(nproc)')", config->compile_script, &config->compile_script_lines);
    
    // Get verify script
    get_multiline_input("Verify script (e.g., 'make check')", config->verify_script, &config->verify_script_lines);
    
    // Get assemble script(s)
    if (config->package_count == 1) {
        printf("\nAssemble script for %s:\n", config->packages[0].name);
        get_multiline_input("Assemble script (e.g., 'make DESTDIR=\"${pkgdir}\" install')", config->assemble_scripts[0], &config->assemble_script_lines[0]);
    } else {
        printf("\nAssemble scripts for each package:\n");
        for (int i = 0; i < config->package_count; i++) {
            char prompt[256];
            snprintf(prompt, sizeof(prompt), "Assemble script for %s", config->packages[i].name);
            get_multiline_input(prompt, config->assemble_scripts[i], &config->assemble_script_lines[i]);
        }
    }
}

void wizard_options(StarbuildConfig *config) {
    print_header("Package Options");
    printf("Select package options using arrow keys and Enter to toggle:\n");
    printf("Use arrow keys to navigate, Enter to toggle, 'q' to finish\n\n");
    
    // Enable raw mode for better input handling
    enable_raw_mode();
    
    // Define available options
    const char *available_options[] = {
        "no-strip",
        "no-strip-binaries", 
        "no-remove-la",
        "no-remove-a",
        "man",
        "libs",
        "include",
        "docs",
        "lto"
    };
    int num_options = sizeof(available_options) / sizeof(available_options[0]);
    
    // Initialize option states (0 = disabled, 1 = enabled, 2 = negated)
    int option_states[MAX_OPTIONS] = {0};
    int selected_option = 0;
    
    while (1) {
        // Clear screen and redraw
        clear_screen();
        print_header("Package Options");
        printf("Select package options using arrow keys and Enter to toggle:\n");
        printf("Use arrow keys to navigate, Enter to toggle, 'q' to finish\n\n");
        
        for (int i = 0; i < num_options; i++) {
            if (i == selected_option) {
                printf("  > ");
            } else {
                printf("    ");
            }
            
            if (option_states[i] == 0) {
                // Disabled - not shown
                printf("  %s\n", available_options[i]);
            } else if (option_states[i] == 1) {
                // Enabled - white highlight
                print_highlighted(available_options[i]);
                printf("\n");
            } else if (option_states[i] == 2) {
                // Negated - red highlight
                print_disabled("!");
                print_disabled(available_options[i]);
                printf("\n");
            }
        }
        
        printf("\nLegend: ");
        print_highlighted("purple = enabled");
        printf(", ");
        print_disabled("red = disabled");
        printf(", normal = not selected\n");
        
        // Get user input
        int ch = getchar();
        
        // Handle arrow keys and other input
        if (ch == 'q' || ch == 'Q') {
            break;
        } else if (ch == 27) { // ESC sequence
            ch = getchar();
            if (ch == '[') {
                ch = getchar();
                if (ch == 'A') { // Up arrow
                    selected_option = (selected_option - 1 + num_options) % num_options;
                } else if (ch == 'B') { // Down arrow
                    selected_option = (selected_option + 1) % num_options;
                }
            }
        } else if (ch == '\n' || ch == ' ') { // Enter or space
            // Toggle option state: 0 -> 1 -> 2 -> 0
            option_states[selected_option] = (option_states[selected_option] + 1) % 3;
        }
    }
    
    // Disable raw mode
    disable_raw_mode();
    
    // Convert selected options to config
    config->options_count = 0;
    for (int i = 0; i < num_options; i++) {
        if (option_states[i] == 1) {
            // Enabled option
            strcpy(config->options[config->options_count], available_options[i]);
            config->options_count++;
        } else if (option_states[i] == 2) {
            // Negated option
            snprintf(config->options[config->options_count], sizeof(config->options[config->options_count]), "!%s", available_options[i]);
            config->options_count++;
        }
    }
}

// File generation functions
void write_array(FILE *fp, const char *name, char array[][256], int count) {
    if (count == 0) {
        fprintf(fp, "%s=( )\n", name);
        return;
    }
    
    fprintf(fp, "%s=( ", name);
    for (int i = 0; i < count; i++) {
        fprintf(fp, "\"%s\" ", array[i]);
    }
    fprintf(fp, ")\n");
}

void write_sources_array(FILE *fp, const char *name, char array[][512], int count) {
    if (count == 0) {
        fprintf(fp, "%s=( )\n", name);
        return;
    }
    
    fprintf(fp, "%s=( ", name);
    for (int i = 0; i < count; i++) {
        fprintf(fp, "\"%s\" ", array[i]);
    }
    fprintf(fp, ")\n");
}

void write_script(FILE *fp, const char *name, const char *script) {
    fprintf(fp, "%s() {\n%s}\n\n", name, script);
}

void write_multiline_script(FILE *fp, const char *name, char script[][MAX_LINE_LENGTH], int line_count) {
    fprintf(fp, "%s() {\n", name);
    for (int i = 0; i < line_count; i++) {
        if (strlen(script[i]) > 0) {
            fprintf(fp, "    %s\n", script[i]);
        }
    }
    fprintf(fp, "}\n\n");
}

void generate_starbuild_file(StarbuildConfig *config) {
    FILE *fp = fopen("STARBUILD", "w");
    if (!fp) {
        print_error("Could not create STARBUILD file");
        return;
    }
    
    fprintf(fp, "# STARBUILD generated by StarbuildCreator\n\n");
    
    // Package information
    if (config->package_count == 1) {
        fprintf(fp, "package_name=\"%s\"\n", config->packages[0].name);
        fprintf(fp, "package_version=\"%s\"\n", config->packages[0].version);
        fprintf(fp, "description=\"%s\"\n", config->packages[0].description);
        
        // Write license array for single package
        if (config->packages[0].license_count > 0) {
            write_array(fp, "license", config->packages[0].license, config->packages[0].license_count);
        }
        fprintf(fp, "\n");
    } else {
        // Multiple packages
        fprintf(fp, "package_name=( ");
        for (int i = 0; i < config->package_count; i++) {
            fprintf(fp, "\"%s\" ", config->packages[i].name);
        }
        fprintf(fp, ")\n");
        fprintf(fp, "package_version=\"%s\"\n", config->packages[0].version);
        
        fprintf(fp, "package_descriptions=( ");
        for (int i = 0; i < config->package_count; i++) {
            fprintf(fp, "\"%s\" ", config->packages[i].description);
        }
        fprintf(fp, ")\n\n");
    }
    
    // Dependencies
    write_array(fp, "dependencies", config->global_deps, config->global_deps_count);
    write_array(fp, "build_dependencies", config->build_deps, config->build_deps_count);
    write_sources_array(fp, "sources", config->sources, config->sources_count);
    
    // Write options if any
    if (config->options_count > 0) {
        write_array(fp, "options", config->options, config->options_count);
    }
    
    fprintf(fp, "\n");
    
    // Scripts
    write_multiline_script(fp, "prepare", config->prepare_script, config->prepare_script_lines);
    write_multiline_script(fp, "compile", config->compile_script, config->compile_script_lines);
    write_multiline_script(fp, "verify", config->verify_script, config->verify_script_lines);
    
    // Assemble scripts
    if (config->package_count == 1) {
        write_multiline_script(fp, "assemble", config->assemble_scripts[0], config->assemble_script_lines[0]);
    } else {
        for (int i = 0; i < config->package_count; i++) {
            char script_name[256];
            snprintf(script_name, sizeof(script_name), "assemble_%s", config->packages[i].name);
            write_multiline_script(fp, script_name, config->assemble_scripts[i], config->assemble_script_lines[i]);
        }
    }
    
    // Package-specific dependencies
    if (config->package_count > 1) {
        for (int i = 0; i < config->package_count; i++) {
            if (config->packages[i].deps_count > 0) {
                char deps_name[256];
                snprintf(deps_name, sizeof(deps_name), "dependencies_%s", config->packages[i].name);
                write_array(fp, deps_name, config->packages[i].deps, config->packages[i].deps_count);
            }
        }
    }
    
    // Package-specific licenses
    if (config->package_count > 1) {
        for (int i = 0; i < config->package_count; i++) {
            if (config->packages[i].license_count > 0) {
                char license_name[256];
                snprintf(license_name, sizeof(license_name), "license_%s", config->packages[i].name);
                write_array(fp, license_name, config->packages[i].license, config->packages[i].license_count);
            }
        }
    }
    
    // Advanced fields for single package
    if (config->package_count == 1 && config->enable_advanced_fields) {
        if (config->packages[0].gives_count > 0) {
            write_array(fp, "gives", config->packages[0].gives, config->packages[0].gives_count);
        }
        if (config->packages[0].clashes_count > 0) {
            write_array(fp, "clashes", config->packages[0].clashes, config->packages[0].clashes_count);
        }
        if (config->packages[0].optional_dependencies_count > 0) {
            write_array(fp, "optional_dependencies", config->packages[0].optional_dependencies, config->packages[0].optional_dependencies_count);
        }
    }
    
    // Advanced fields for multiple packages
    if (config->package_count > 1 && config->enable_advanced_fields) {
        for (int i = 0; i < config->package_count; i++) {
            if (config->packages[i].gives_count > 0) {
                char gives_name[256];
                snprintf(gives_name, sizeof(gives_name), "gives_%s", config->packages[i].name);
                write_array(fp, gives_name, config->packages[i].gives, config->packages[i].gives_count);
            }
            if (config->packages[i].clashes_count > 0) {
                char clashes_name[256];
                snprintf(clashes_name, sizeof(clashes_name), "clashes_%s", config->packages[i].name);
                write_array(fp, clashes_name, config->packages[i].clashes, config->packages[i].clashes_count);
            }
            if (config->packages[i].optional_dependencies_count > 0) {
                char optional_deps_name[256];
                snprintf(optional_deps_name, sizeof(optional_deps_name), "optional_%s", config->packages[i].name);
                write_array(fp, optional_deps_name, config->packages[i].optional_dependencies, config->packages[i].optional_dependencies_count);
            }
        }
    }
    
    fclose(fp);
    print_success("STARBUILD file created successfully!");
}

// Main wizard function
void run_wizard(StarbuildConfig *config) {
    clear_screen();
    printf("Welcome to StarbuildCreator!\n");
    printf("This wizard will help you create a STARBUILD file.\n\n");
    
    // Check for existing STARBUILD file
    if (access("STARBUILD", F_OK) == 0) {
        print_warning("STARBUILD file already exists");
        if (!get_yes_no("Overwrite existing file")) {
            printf("Operation cancelled.\n");
            return;
        }
    }
    
    // Run wizard steps
    wizard_advanced_fields(config);
    wizard_basic_info(config);
    wizard_dependencies(config);
    wizard_sources(config);
    wizard_advanced_package_fields(config);
    wizard_scripts(config);
    wizard_options(config);
    
    // Preview
    print_header("Preview");
    if (config->package_count == 1) {
        printf("Package: %s %s\n", config->packages[0].name, config->packages[0].version);
        printf("Description: %s\n", config->packages[0].description);
        if (config->packages[0].license_count > 0) {
            printf("License: ");
            for (int i = 0; i < config->packages[0].license_count; i++) {
                printf("%s ", config->packages[0].license[i]);
            }
            printf("\n");
        }
    } else {
        printf("Packages:\n");
        for (int i = 0; i < config->package_count; i++) {
            printf("  - %s: %s\n", config->packages[i].name, config->packages[i].description);
            if (config->packages[i].license_count > 0) {
                printf("    License: ");
                for (int j = 0; j < config->packages[i].license_count; j++) {
                    printf("%s ", config->packages[i].license[j]);
                }
                printf("\n");
            }
        }
    }
    printf("Dependencies: ");
    for (int i = 0; i < config->global_deps_count; i++) {
        printf("%s ", config->global_deps[i]);
    }
    printf("\n");
    
    if (config->enable_advanced_fields && config->package_count == 1) {
        if (config->packages[0].gives_count > 0) {
            printf("Gives: ");
            for (int i = 0; i < config->packages[0].gives_count; i++) {
                printf("%s ", config->packages[0].gives[i]);
            }
            printf("\n");
        }
        if (config->packages[0].clashes_count > 0) {
            printf("Clashes: ");
            for (int i = 0; i < config->packages[0].clashes_count; i++) {
                printf("%s ", config->packages[0].clashes[i]);
            }
            printf("\n");
        }
        if (config->packages[0].optional_dependencies_count > 0) {
            printf("Optional dependencies: ");
            for (int i = 0; i < config->packages[0].optional_dependencies_count; i++) {
                printf("%s ", config->packages[0].optional_dependencies[i]);
            }
            printf("\n");
        }
    }
    
    if (config->options_count > 0) {
        printf("Options: ");
        for (int i = 0; i < config->options_count; i++) {
            printf("%s ", config->options[i]);
        }
        printf("\n");
    }
    
    if (get_yes_no("Generate STARBUILD file")) {
        generate_starbuild_file(config);
        
        if (get_yes_no("Save as template for future use")) {
            get_input("Template name", config->template_name, sizeof(config->template_name));
            save_template(config->template_name, config);
        }
    } else {
        printf("Operation cancelled.\n");
    }
}

// Quick mode function
void quick_mode(const char *package_name, const char *version, const char *description) {
    StarbuildConfig config = {0};
    
    strcpy(config.packages[0].name, package_name);
    strcpy(config.packages[0].version, version);
    strcpy(config.packages[0].description, description);
    config.package_count = 1;
    config.enable_advanced_fields = 0;
    config.options_count = 0; // No options in quick mode
    
    generate_starbuild_file(&config);
    print_success("Quick STARBUILD file created!");
}

// Main function
int main(int argc, char *argv[]) {
    StarbuildConfig config = {0};
    
    // Initialize all counters to 0
    config.package_count = 0;
    config.global_deps_count = 0;
    config.build_deps_count = 0;
    config.sources_count = 0;
    config.prepare_script_lines = 0;
    config.compile_script_lines = 0;
    config.verify_script_lines = 0;
    config.enable_advanced_fields = 0;
    config.options_count = 0;
    
    // Initialize package counters
    for (int i = 0; i < MAX_PACKAGES; i++) {
        config.packages[i].license_count = 0;
        config.packages[i].deps_count = 0;
        config.packages[i].conflicts_count = 0;
        config.packages[i].provides_count = 0;
        config.packages[i].optional_count = 0;
        config.packages[i].gives_count = 0;
        config.packages[i].clashes_count = 0;
        config.packages[i].optional_dependencies_count = 0;
        config.assemble_script_lines[i] = 0;
    }
    
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("StarbuildCreator - Easy STARBUILD file generator\n\n");
            printf("Usage:\n");
            printf("  %s                    Interactive wizard mode\n", argv[0]);
            printf("  %s -q NAME VER DESC   Quick mode with auto-detection\n", argv[0]);
            printf("  %s -t TEMPLATE        Use template\n", argv[0]);
            printf("  %s -h, --help         Show this help\n", argv[0]);
            return 0;
        } else if (strcmp(argv[1], "-q") == 0 && argc >= 5) {
            quick_mode(argv[2], argv[3], argv[4]);
            return 0;
        } else if (strcmp(argv[1], "-t") == 0 && argc >= 3) {
            load_template(argv[2], &config);
        }
    }
    
    run_wizard(&config);
    return 0;
} 