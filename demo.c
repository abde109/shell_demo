#include "shell.h"

void _free(char **buffer)
{
    int counter = 0;

    if (buffer)
    {
        for (counter = 0; buffer[counter]; counter++)
            free(buffer[counter]);
        free(buffer);
    }
}

void my_exit(void)
{
    exit(EXIT_SUCCESS);
}

void my_exit_status(char **parameters)
{
    int exitStatus = atoi(parameters[1]);
    free(parameters);
    exit(exitStatus);
}

void _cd(char **parameters)
{
    char *homeDirectory;

    if (!parameters[0])
        _perror("cd: missing argument\n");
    else if (!parameters[1])
    {
        homeDirectory = getenv("HOME");
        if (chdir(homeDirectory) != 0)
            _perror("cd: No such file or directory\n");
    }
    else
    {
        if (chdir(parameters[1]) != 0)
            _perror("cd: No such file or directory\n");
    }
}

void env_shell(void)
{
    int counter = 0;

    while (environ[counter] != NULL)
    {
        _print(environ[counter]);
        _print("\n");
        counter++;
    }
}

int set_env(char *varName, char *varValue)
{
    char *environmentVar;
    int length;

    length = strlen(varName) + strlen(varValue) + 2;
    environmentVar = malloc(length);
    if (environmentVar == NULL)
        return (-1);
    snprintf(environmentVar, length, "%s=%s", varName, varValue);
    if (putenv(environmentVar) != 0)
        return (-1);
    return (0);
}

int unset_env(char *varName)
{
    if (unsetenv(varName) != 0)
        return (-1);
    return (0);
}

void execute_command(char **parameters, char *parameter)
{
    int processStatus;
    pid_t processId;

    if (_strcmp(parameters[0], "cd") == 0)
    {
        _cd(parameters);
        return;
    }
    else if (strcmp(parameters[0], "env") == 0)
    {
        env_shell();
        return;
    }
    processId = fork();
    if (processId == 0)
    {
        execvp(parameters[0], parameters);
        if (errno == ENOENT)
        {
            _perror(parameter);
            _perror(": No such file or directory\n");
            exit(127);
        }
        else
        {
            _perror("Command not found.\n");
            exit(127);
        }
    }
    else if (processId > 0)
        waitpid(processId, &processStatus, WUNTRACED);
    else if (processId < 0)
        _perror("failed to fork\n");
}

char *search_path(char *fileName)
{
    char *path, *pathEnvironment, *fullPath;

    if (fileName[0] == '/')
    {
        if (access(fileName, F_OK) == 0)
            return (realpath(fileName, NULL));
        else
            return (NULL);
    }

    pathEnvironment = malloc(MAX_PATH_LEN * sizeof(char));
    snprintf(pathEnvironment, MAX_PATH_LEN, "/bin:%s", getenv("PATH"));
    path = strtok(pathEnvironment, ":");
    fullPath = malloc(MAX_PATH_LEN * sizeof(char));

    while (path != NULL)
    {
        snprintf(fullPath, MAX_PATH_LEN, "%s/%s", path, fileName);
        if (access(fullPath, F_OK) == 0)
        {
            free(pathEnvironment);
            return (realpath(fullPath, NULL));
        }
        path = strtok(NULL, ":");
    }

    free(pathEnvironment);
    free(fullPath);
    return (NULL);
}

char **tokenize(char *line)
{
    int length = 0;
    int bufferCapacity = 15;
    char *delimiter, *token, **tokens = malloc(bufferCapacity * sizeof(char *));

    if (!tokens)
    {
        _perror("ERROR token malloc failed");
        exit(1);
    }

    delimiter = " \t\r\n";
    token = strtok(line, delimiter);

    while (token != NULL)
    {
        tokens[length] = token;
        length++;

        if (length >= bufferCapacity)
        {
            bufferCapacity = (int)(bufferCapacity * 1.5);
            tokens = realloc(tokens, bufferCapacity * sizeof(char *));
        }

        token = strtok(NULL, delimiter);
    }

    tokens[length] = NULL;
    return (tokens);
}

char *read_line()
{
    char *inputLine = NULL;
    size_t bufferSize = 0;

    if (getline(&inputLine, &bufferSize, stdin) == -1)
    {
        free(inputLine);
        exit(1);
    }
    return (inputLine);
}

int _print(char *text)
{
    return (write(STDOUT_FILENO, text, _strlen(text)));
}

int _perror(char *errorMessage)
{
    return (write(STDERR_FILENO, errorMessage, _strlen(errorMessage)));
}

void prompt(char **arguments __attribute__((unused)))
{
    char inputLine[INPUT_LEN];
    int terminalCheck = isatty(STDIN_FILENO);

    while (1)
    {
        if (terminalCheck)
            _print(PROMPT);
        if (fgets(inputLine, INPUT_LEN, stdin) == NULL)
            break;
        file_prompt(inputLine, arguments);
    }
}

void run_commands_from_file(const char *fileName, char **arguments)
{
    char inputLine[MAX_LINE_LENGTH];
    FILE *filePointer = fopen(fileName, "r");

    if (!filePointer)
    {
        _perror(arguments[0]);
        _perror(": 0: Can't open");
        _perror("\n");
        exit(127);
    }

    while (fgets(inputLine, MAX_LINE_LENGTH, filePointer) != NULL)
    {
        file_prompt(inputLine, arguments);
    }

    fclose(filePointer);
}

void file_prompt(char *inputLine, char **arguments)
{
    char **tokens, previousChar, *linePointer, *comment;
    int quoteFlag;

    previousChar = '\0';
    comment = NULL;
    quoteFlag = 0;
    linePointer = inputLine;
    while (*linePointer != '\0')
    {
        if (*linePointer == '"' && previousChar != '\\')
            quoteFlag = !quoteFlag;
        else if (*linePointer == '#' && previousChar != '\\' && !quoteFlag)
        {
            if (previousChar == ' ')
            {
                comment = linePointer;
                break;
            }
        }
        previousChar = *linePointer;
        linePointer++;
    }
    if (comment != NULL)
        *comment = '\0';
    inputLine[strcspn(inputLine, "\n")] = '\0';
    tokens = tokenize(inputLine);
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
        execute_command(tokens, arguments[0]);
    }
    free(tokens);
}

int main(int argCount, char **arguments)
{
    if (argCount == 1)
        prompt(arguments);
    if (argCount == 2)
        run_commands_from_file(arguments[1], arguments);
    return (EXIT_SUCCESS);
}

char *_strtok(char *str, char delimiter)
{
    char *inputStr = NULL;
    char *resultStr;
    int counter = 0;

    if (str != NULL)
        inputStr = str;

    if (inputStr == NULL)
        return (NULL);

    resultStr = malloc(strlen(inputStr) + 1);

    for (; inputStr[counter] != '\0'; counter++)
    {
        if (inputStr[counter] != delimiter)
            resultStr[counter] = inputStr[counter];
        else
        {
            resultStr[counter] = '\0';
            inputStr = inputStr + counter + 1;
            return (resultStr);
        }
    }

    resultStr[counter] = '\0';
    inputStr = NULL;

    return (resultStr);
}

int _strlen(char *str)
{
    int length = 0;

    if (!str)
        return (0);

    while (*str++)
        length++;
    return (length);
}

int _strcmp(char *str1, char *str2)
{
    while (*str1 && *str2)
    {
        if (*str1 != *str2)
            return (*str1 - *str2);
        str1++;
        str2++;
    }
    if (*str1 == *str2)
        return (0);
    else
        return (*str1 < *str2 ? -1 : 1);
}

int _puts(char *text)
{
    int counter;

    if (!(text))
        return (0);
    for (counter = 0; text[counter]; counter++)
        write(STDOUT_FILENO, &text[counter], 1);
    return (counter);
}
