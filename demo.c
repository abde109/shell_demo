#include "shell.h"

void _free(char **buf)
{
    int i = 0;

    if (buf)
    {
        for (i = 0; buf[i]; i++)
            free(buf[i]);
        free(buf);
    }
}



void my_exit(void)
{
    exit(EXIT_SUCCESS);
}

void my_exit_status(char **args)
{
    int status = atoi(args[1]);
    free(args);
    exit(status);
}

void _cd(char **args)
{
    char *home;

    if (!args[0])
        _perror("cd: missing argument\n");
    else if (!args[1])
    {
        home = getenv("HOME");
        if (chdir(home) != 0)
            _perror("cd: No such file or directory\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
            _perror("cd: No such file or directory\n");
    }
}

void env_shell(void)
{
    int i = 0;

    while (environ[i] != NULL)
    {
        _print(environ[i]);
        _print("\n");
        i++;
    }
}



int set_env(char *name, char *value)
{
    char *env_var;
    int len;

    len = strlen(name) + strlen(value) + 2;
    env_var = malloc(len);
    if (env_var == NULL)
        return (-1);
    snprintf(env_var, len, "%s=%s", name, value);
    if (putenv(env_var) != 0)
        return (-1);
    return (0);
}

int unset_env(char *name)
{
    if (unsetenv(name) != 0)
        return (-1);
    return (0);
}



void execute_command(char **args, char *arg)
{
    int status;
    pid_t pid;

    if (_strcmp(args[0], "cd") == 0)
    {
        _cd(args);
        return;
    }
    else if (strcmp(args[0], "env") == 0)
    {
        env_shell();
        return;
    }
    pid = fork();
    if (pid == 0)
    {
        execvp(args[0], args);
        if (errno == ENOENT)
        {
            _perror(arg);
            _perror(": No such file or directory\n");
            exit(127);
        }
        else
        {
            _perror("Command not found.\n");
            exit(127);
        }
    }
    else if (pid > 0)
        waitpid(pid, &status, WUNTRACED);
    else if (pid < 0)
        _perror("failed to fork\n");
}

char *search_path(char *filename)
{
    char *path, *path_env, *full_path;

    if (filename[0] == '/')
    {
        if (access(filename, F_OK) == 0)
            return (realpath(filename, NULL));
        else
            return (NULL);
    }

    path_env = malloc(MAX_PATH_LEN * sizeof(char));
    snprintf(path_env, MAX_PATH_LEN, "/bin:%s", getenv("PATH"));
    path = strtok(path_env, ":");
    full_path = malloc(MAX_PATH_LEN * sizeof(char));

    while (path != NULL)
    {
        snprintf(full_path, MAX_PATH_LEN, "%s/%s", path, filename);
        if (access(full_path, F_OK) == 0)
        {
            free(path_env);
            return (realpath(full_path, NULL));
        }
        path = strtok(NULL, ":");
    }

    free(path_env);
    free(full_path);
    return (NULL);
}



char **tokenize(char *line)
{
    int len = 0;
    int capacity = 15;
    char *delim, *token, **tokens = malloc(capacity * sizeof(char *));

    if (!tokens)
    {
        _perror("ERROR token malloc failed");
        exit(1);
    }

    delim = " \t\r\n";
    token = strtok(line, delim);

    while (token != NULL)
    {
        tokens[len] = token;
        len++;

        if (len >= capacity)
        {
            capacity = (int)(capacity * 1.5);
            tokens = realloc(tokens, capacity * sizeof(char *));
        }

        token = strtok(NULL, delim);
    }

    tokens[len] = NULL;
    return (tokens);
}

char *read_line()
{
    char *line = NULL;
    size_t buf = 0;

    if (getline(&line, &buf, stdin) == -1)
    {
        free(line);
        exit(1);
    }
    return (line);
}



int _print(char *string)
{
    return (write(STDOUT_FILENO, string, _strlen(string)));
}

int _perror(char *err)
{
    return (write(STDERR_FILENO, err, _strlen(err)));
}



void prompt(char **argv __attribute__((unused)))
{
    char line[INPUT_LEN];
    int is_terminal = isatty(STDIN_FILENO);

    while (1)
    {
        if (is_terminal)
            _print(PROMPT);
        if (fgets(line, INPUT_LEN, stdin) == NULL)
            break;
        file_prompt(line, argv);
    }
}



void run_commands_from_file(const char *filename, char **argv)
{
    char line[MAX_LINE_LENGTH];
    FILE *fp = fopen(filename, "r");

    if (!fp)
    {
        _perror(argv[0]);
        _perror(": 0: Can't open");
        _perror("\n");
        exit(127);
    }

    while (fgets(line, MAX_LINE_LENGTH, fp) != NULL)
    {
        file_prompt(line, argv);
    }

    fclose(fp);
}

void file_prompt(char *line, char **argv)
{
    char **tokens, prev, *l, *cmt;
    int if_quote;

    prev = '\0';
    cmt = NULL;
    if_quote = 0;
    l = line;
    while (*l != '\0')
    {
        if (*l == '"' && prev != '\\')
            if_quote = !if_quote;
        else if (*l == '#' && prev != '\\' && !if_quote)
        {
            if (prev == ' ')
            {
                cmt = l;
                break;
            }
        }
        prev = *l;
        l++;
    }
    if (cmt != NULL)
        *cmt = '\0';
    line[strcspn(line, "\n")] = '\0';
    tokens = tokenize(line);
    if (tokens[0] != NULL)
    {
        if (strcmp(tokens[0], "exit") == 0)
        {
            if (tokens[1] != NULL)
            {
                my_exit_status(tokens);
            }
            free(tokens);
            my_exit();
        }
        execute_command(tokens, argv[0]);
    }
    free(tokens);
}



int main(int argc, char **argv)
{
    if (argc == 1)
        prompt(argv);
    if (argc == 2)
        run_commands_from_file(argv[1], argv);
    return (EXIT_SUCCESS);
}



char *_strtok(char *s, char d)
{
    char *input = NULL;
    char *result;
    int i = 0;

    if (s != NULL)
        input = s;

    if (input == NULL)
        return (NULL);

    result = malloc(strlen(input) + 1);

    for (; input[i] != '\0'; i++)
    {
        if (input[i] != d)
            result[i] = input[i];
        else
        {
            result[i] = '\0';
            input = input + i + 1;
            return (result);
        }
    }

    result[i] = '\0';
    input = NULL;

    return (result);
}

int _strlen(char *s)
{
    int i = 0;

    if (!s)
        return (0);

    while (*s++)
        i++;
    return (i);
}

int _strcmp(char *s1, char *s2)
{
    while (*s1 && *s2)
    {
        if (*s1 != *s2)
            return (*s1 - *s2);
        s1++;
        s2++;
    }
    if (*s1 == *s2)
        return (0);
    else
        return (*s1 < *s2 ? -1 : 1);
}

int _puts(char *str)
{
    int i;

    if (!(str))
        return (0);
    for (i = 0; str[i]; i++)
        write(STDOUT_FILENO, &str[i], 1);
    return (i);
}
