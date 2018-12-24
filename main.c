/*
 * \file main.c
 * \author Pierre Johner Mikael Cervera
 * \brief Simplified command line interpreter
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define OPEN_ERROR  "Erreur d'ouverture\x0d\x0a"
#define READ_ERROR  "Erreur de lecture\x0a\x0a"
#define WRITE_ERROR "Erreur d'écriture\x0a\x0a"
#define PROFIL_ERROR "Erreur de fichier\x0a\x0a"
#define CMD_ERROR "Commande Inconnue\x0a\x0a"
#define ARGS_ERROR "Argument Invalide\x0a\x0a"
#define CHDIR_ERROR "Chdir Failed\x0a\x0a"
#define ALIASLOOP_ERROR "Alias Loop\x0a\x0a"
#define ALIASDISPLAY_ERROR "Alias Display Empty\x0a\x0a"
#define EXECV_ERROR "EXECV Erreur\x0a\x0a"

//Struct for the alias
typedef struct Element Element;
struct Element
{
    char* label;
    char* data;
    Element *next;
};
/**
Declaration shell function
*/
int cd(char **args);
int pwd(char **args);
int alias(char **args);
int exit_p(char **args);
/*************************************/

/**
 Constant
*/
// list of function for the call
char *commandes[] =
{
    "cd",
    "pwd",
    "alias",
    "exit_p"
};
int (*func_select[]) (char **) =
{
    &cd,
    &pwd,
    &alias,
    &exit_p
};

//Global int for readability
int SORTIE_ERREUR = 2;
int SORTIE_STANDARD = 1;
int ENTREE_STANDARD = 0;

//List of alias
Element* aliasList;
/*************************************/

/**
* Create a new Element
*@param char* data
*@param char* label
*@return Element*
*/
Element* NewElement(char* data,char* label)
{
    Element* item = malloc(sizeof(Element));
    if (item == NULL)
    {
        exit(EXIT_FAILURE);
    }
    item->next = NULL;
    item->data = malloc(sizeof(char*)*strlen(data));
    strcat(item->data,data);
    item->label = malloc(sizeof(char*)*strlen(label));
    strcat(item->label,label);
    return item;
}

/**
* Search on list by label
*@param Element* list where do research
*@param char* label
*@return Element* Element found
*/
Element* SearchElement(Element* list,char* label)
{
    if(list != NULL)
    {
        Element* tmp = list;
        if(strcmp(tmp->label,label)==0)
            return tmp;

        while(tmp->next != NULL)
        {
            tmp = tmp->next;
            if(strcmp(tmp->label,label)==0)
                return tmp;
        }
    }
    return NULL;
}

/**
* Add a Element to list
*@param Element* list where add
*@param char* label
*@param char* data
*@return Element* new list
*/
Element* AddElement(Element* list,char* data,char* label)
{
    Element* item = SearchElement(list,label);
    if(item==NULL) //not in the list
    {
        Element* newElement = NewElement(data,label);
        newElement->next = list;
        return newElement;
    }
    else
    {
        item->data = data;
        return list;
    }
}

/**
* Display  the list
*@param Element* list to display
*/
void DisplayElement(Element* list)
{
    if(list != NULL)
    {
        Element* tmp = list;
        write(SORTIE_STANDARD,"alias ",strlen("alias "));
        write(SORTIE_STANDARD,list->label,strlen(list->label));
        write(SORTIE_STANDARD,"='",strlen("='"));
        write(SORTIE_STANDARD,list->data,strlen(list->data));
        write(SORTIE_STANDARD,"'\n",strlen("'\n"));

        while(tmp->next != NULL)
        {
            tmp = tmp->next;
            write(SORTIE_STANDARD,"alias ",strlen("alias "));
            write(SORTIE_STANDARD,tmp->label,strlen(tmp->label));
            write(SORTIE_STANDARD,"='",strlen("='"));
            write(SORTIE_STANDARD,tmp->data,strlen(tmp->data));
            write(SORTIE_STANDARD,"'\n",strlen("'\n"));

        }
    }
    else
        write(SORTIE_ERREUR,ALIASDISPLAY_ERROR,strlen(ALIASDISPLAY_ERROR));
}

/**
* Detect if the new char* create a loop
*@param Element* list where search
*@param char* start
*@return int 1 if loop 0 if not
*/
int isLoop(Element* list,char* start)
{
    Element* item;
    int token = 0;
    char* tmp = start;
    while(!token)
    {
        item =SearchElement(list,tmp);
        if(item!=NULL)
        {
            if(strcmp(item->data,start))
            {
                return 1;
            }
        }
        else
        {
            return 0;
        }
    }
}

/**
* Split a line
*@param char* line to split
*@param char* delimiter
*@return char** list of char*
*/
char** split_command(char* line,char* token_delim)
{
    int buffer = 64;
    int position = 0;
    char **tokens = malloc(buffer * sizeof(char*));
    char *token;

    token = strtok(line, token_delim);
    while (token != NULL)
    {
        tokens[position] = token;
        position++;

        if (position >= buffer)
        {
            buffer += buffer;
            tokens = realloc(tokens, buffer * sizeof(char*));
        }

        token = strtok(NULL,token_delim);
    }
    tokens[position] = NULL;
    return tokens;

}

/**
* Count number of command
*@return int number of command
*/
int num_commands()
{
    return sizeof(commandes) / sizeof(char *);
}

/**
* execut a line of command
*@param char** line of command
*@return int continu 0 or exit 1
*/
int execute(char **args)
{
    if (args[0] == NULL)
    {
        write(SORTIE_ERREUR,CMD_ERROR,strlen(CMD_ERROR));
        return 0;
    }

    if(!strcmp(args[0],"exit"))
        args[0]="exit_p";

    for (int i = 0; i < num_commands(); i++)
    {
        if (strcmp(args[0], commandes[i]) == 0)
        {
            return (*func_select[i])(args);
        }
    }

    linux_command(args)!= 0;
    return 0;
}

/**
* Format command
*@param char* line of command
*@return int continu 0 or exit 1
*/
int interpreter(char* commande)
{
    if(!strcmp(commande,"\n"))
        return 0;
    char** args = split_command(commande," \t\r\n\a");
    replaceAlias(args);
    replaceVar(args);

    if(args[1]==NULL && strstr(args[0],"=")!=NULL)
        return funcVar(args[0]);
    else
        return execute(args);
}

/**
* Set environnement varaible
*@param char* format x=y
*@return int status
*/
int funcVar(char*args)
{
    char** datas = split_command(args,"=");
    return setenv(datas[0],datas[1],1);
}

/**
* Get environnement varaible
*@param char** list of command
*/
void replaceVar(char** args)
{
    //support '  and back quote ?
    int numberElements = 0;
    while(args[numberElements]!=NULL)
    {
        if(strstr(args[numberElements],"$")!=NULL)
        {
            char** items = split_command(args[numberElements],"$");
            if(items[1]!=NULL)
            {
                char* newItem = items[0];
                strcat(newItem,getenv(items[1]));
                args[numberElements]= newItem;
            }
            else
            {
                args[numberElements] = getenv(items[0]);
            }
        }
        numberElements++;
    }
}

/**
* Replace Alias of the list od command
*@param char** list of command
*/
void replaceAlias(char** args)
{
    int numberElements = 0;
    int token = 1;
    while(token)
    {
        token= 0;
        numberElements = 0;
        while(args[numberElements]!=NULL)
        {
            Element* item = SearchElement(aliasList,args[numberElements]);
            if(item!=NULL)
            {
                args[numberElements]=item->data;
                token=1;
            }
            numberElements++;
        }
    }
}

/**
  Replaces shell commands
*/
int cd(char**args)
{
    if(args[1]!=NULL)
    {
        int error = chdir(args[1]);
        if(error!=0)
            write(SORTIE_ERREUR,CHDIR_ERROR,strlen(CHDIR_ERROR));
    }
    else
        write(SORTIE_ERREUR,ARGS_ERROR,strlen(ARGS_ERROR));

    return 0;
}
int pwd(char**args)
{
    char* home = getenv("HOME");
    write(SORTIE_STANDARD,home,strlen(home));
    write(SORTIE_STANDARD,"\n",strlen("\n"));
    return 0;
}
int alias(char**args)
{
    if(args[1]!=NULL)
    {
        char** datas = split_command(args[1],"=");
        if(!isLoop(aliasList,datas[1]))
            aliasList=AddElement(aliasList,datas[1],datas[0]);
        else
            write(SORTIE_ERREUR,ALIASLOOP_ERROR,strlen(ALIASLOOP_ERROR));
    }
    else //affiche tous les alias
    {
        DisplayElement(aliasList);
    }
    return 0;
}
int exit_p(char**args)
{
    return 1;
}
/*************************************/

/**
* Execut other shell commands
*@param char** list of command
*@return int status
*/
int linux_command(char **args)
{
    pid_t pid = fork();
    int status;
    if(pid==0) //fils
    {
        int token = 0;
        char** path = split_command(getenv("PATH"),":");

        for(int i =0; i<3; i++)
        {
            strcat(path[i],"/");
            strcat(path[i],args[0]);
            if(execv(path[i],args)<0)//execve pour propager les variables d'environement
                token++;
        }
        if(token>2)
            write(SORTIE_ERREUR,EXECV_ERROR,strlen(EXECV_ERROR));
        return 0;
    }
    if(wait(&status)!= pid) //pere
    {
        return 0;
    }
}

int main(int argc, char *argv[], char *env[])
{
    int id = getuid();
    char* work;
    char* home;
    char* path;
    char buffer[256];

    work = getpwuid(id)->pw_dir;

    int fd;
    strcat(work,"/profil");

    fd = open(work,O_RDONLY);

    if(fd<1)
    {
        write(SORTIE_ERREUR,OPEN_ERROR,strlen(OPEN_ERROR));
    }
    else
    {
        int lu;
        lu=read(fd,buffer,sizeof(buffer));
        if(lu<0)
        {
            write(SORTIE_ERREUR,READ_ERROR,strlen(READ_ERROR));
        }
        close(fd);
    }


    int tokenEntry = 1;

    char** items = split_command(buffer,"\n");
    if(items[0] != NULL)
        home = items[0];
    else
        tokenEntry = 0;
    if(items[1] != NULL)
        path = items[1];
    else
        tokenEntry = 0;


    if(tokenEntry == 0)
        write(SORTIE_ERREUR,PROFIL_ERROR,strlen(PROFIL_ERROR));
    else
    {
        if(path[0]!='P')
        {
            char* tmp = home;
            strcpy(home,path);
            strcpy(path,tmp);
        }
    }
    funcVar(home);
    funcVar(path);

    int finish = 0;
    while(!finish)
    {
        char readValueBuffer[100];
        memset(readValueBuffer,0,sizeof(readValueBuffer));
        char commande[100];
        memset(commande,0,sizeof(commande));
        int lu,err;
        err=0;
        do
        {
            lu=read(ENTREE_STANDARD,readValueBuffer,sizeof(readValueBuffer));
            if(lu<0)
            {
                write(SORTIE_ERREUR,READ_ERROR,strlen(READ_ERROR));
                err=1;
            }
            if(lu>0)
            {
                strcat(commande,readValueBuffer);
            }
        }
        while(lu==sizeof(readValueBuffer) && err==0);
        finish = interpreter(commande);
    }

    return 0;
}
