#include <stdio.h>    /* entrées/sorties */

#include <unistd.h>   /* primitives de base : fork, ...*/

#include <stdlib.h>   /* exit */

#include <signal.h>

#include <sys/wait.h> /* wait */

#include "readcmd.h"

#include <stdbool.h>

#include <string.h>

#include <fcntl.h>

struct command_struct * commands_list = NULL;
int id_command = 0;
int pid_retour;
bool foreground_active = false;

struct command_struct {
  int pid;
  char ** command;
  bool is_backgrounded;
  bool is_running;
};


// Gestion des commandes

// Redirige les entrées et sorties standard
void redirect(char * in, char * out) {
  if (in != NULL) {
    int filein = open(in, O_RDONLY);
    dup2(filein, STDIN_FILENO);
    close(filein);
  }
  if (out != NULL) {
    int fileout = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    dup2(fileout, STDOUT_FILENO);
    close(fileout);
  }
}

// Ajouts et suppression des commandes dans la liste des commande 

// Permet d'ajouter une commande à la liste des commandes
void add_command_to_list(int c_pid, char ** cmd, bool is_backgrounded) {
  // Copier la commande dans une nouvelle zone mémoire 
  int len = 0;
  while (cmd[len] != NULL) len++;
  char ** new_cmd = malloc(sizeof(char * ) * (len + 1));
  for (int i = 0; i < len; i++) {
    new_cmd[i] = malloc(sizeof(char) * (strlen(cmd[i]) + 1));
    strcpy(new_cmd[i], cmd[i]);
  }
  new_cmd[len] = NULL;

  struct command_struct new_command = {
    c_pid,
    new_cmd,
    is_backgrounded,
    true
  };
  // On ajoute la commande en tâche de fond dans un tableau (avec allocation dynamique)
  commands_list = realloc(commands_list, (id_command + 1) * sizeof(struct command_struct));
  commands_list[id_command] = new_command;
  id_command++;
}

// Permet de supprimer une commande de la liste des commandes
void remove_command_from_list(int c_pid) {
  for (int i = 0; i < id_command; i++) {
    if (commands_list[i].pid == c_pid) {
      for (int j = i; j < id_command - 1; j++) {
        commands_list[j] = commands_list[j + 1];
      }
      id_command--;
      commands_list = realloc(commands_list, id_command * sizeof(struct command_struct));
    }
  }
}

// Tests sur les commandes

//Verifie si la commande est valide et affiche un message d'erreur si ce n'est pas le cas
bool verify_command(struct cmdline * l) {
  if (l == NULL) {
    printf("Processus en attente sur readcmd à reçu un signal d'appels successifs");
    return false;
  } else if (l -> err != NULL) {
    printf("Erreur au niveau de readcmd : %s\n", l -> err);
    return false;
  } else if (l -> seq[0] == NULL) {
    printf("Commande vide\n");
    return false;
  } else {
    return true;
  }
}

// Permet de savoir si une commande est dans la liste des commandes
bool is_in_command_list(int shell_id) {
  if (shell_id >= id_command) {
    printf("Commande non trouvée\n");
    return false;
  }
  return true;
}


// Gestion des commandes en id

// Permet de trouver l'id d'une commande à partir de son pid
int pid_to_id(int pid) {
  for (int i = 0; i < id_command; i++) {
    if (commands_list[i].pid == pid) {
      return i;
    }
  }
  return -1;
}

void sj(int id_shell) {
  commands_list[id_shell].is_running = false;
  kill(commands_list[id_shell].pid, SIGSTOP);
}

void bg(int id_shell) {
  commands_list[id_shell].is_running = true;
  commands_list[id_shell].is_backgrounded = true;
  kill(commands_list[id_shell].pid, SIGCONT);
}

void lj() {
  if (id_command == 0) {
    printf("Aucune commande en cours\n");
  }
  for (int i = 0; i < id_command; i++) {
    printf("Commande numéro %d\n", i);
    printf("PID : %d\n", commands_list[i].pid);
    printf("Etat : %s\n", commands_list[i].is_running ? "Actif" : "Suspendu");

    printf("Commande :");
    for (int j = 0; commands_list[i].command[j] != NULL; j++) {
      printf(" %s", commands_list[i].command[j]);
    }
    printf("\n");

  }
}

void fg(int id_shell) {

  commands_list[id_shell].is_running = true;
  kill(commands_list[id_shell].pid, SIGCONT);
  commands_list[id_shell].is_backgrounded = false;
  foreground_active = true;
  while (foreground_active != false) {
    pause();
  }
}



// Execution des commandes


// Permet d'exécuter une commande dans un fork
void fork_and_exec(char ** cmd, bool is_backgrounded, int pipe1[2], int pipe2[2], struct cmdline * l, int i) {

  //gestion du pipe
  if (l -> seq[i + 1] != NULL) {
    pipe(pipe2);
  }

  int c_pid = fork();
  int CodeRetour = 0;

  if (c_pid == -1) {

    printf("Erreur : Le fork a échoué\n");
    exit(1);

  } else if (c_pid == 0) {
    // FILS

    // On cache le signaux 
    sigset_t set_cache;
    sigemptyset( & set_cache);
    sigaddset( & set_cache, SIGTSTP);
    sigaddset( & set_cache, SIGINT);
    sigprocmask(SIG_SETMASK, & set_cache, NULL);

    if (l -> seq[i + 1] != NULL) {
      // commande suivante
      close(pipe2[0]);
      dup2(pipe2[1], 1);
      close(pipe2[1]);
    }

    if (i != 0) {
      // commande précédente
      close(pipe1[1]);
      dup2(pipe1[0], 0);
      close(pipe1[0]);
    }

    redirect(l -> in, l -> out);

    CodeRetour = execvp(cmd[0], cmd);

    if (CodeRetour == -1) {
      exit(1);
    }

  } else {
    // PERE
    if (i != 0) {
      // On ferme le pipe précédent
      close(pipe1[0]);
      close(pipe1[1]);
    }
    if (l -> seq[i + 1] != NULL) {
      // On transfert le pipe suivant dans le pipe précédent
      pipe1[0] = pipe2[0];
      pipe1[1] = pipe2[1];
    }

    add_command_to_list(c_pid, cmd, is_backgrounded);

  }
}


// Permet d'exécuter une commande en general
void command_executer(char ** cmd, bool is_backgrounded, int pipe1[2], int pipe2[2], struct cmdline * l, int i) {

  if (strcmp(cmd[0], "exit") == 0) {
    exit(0);
  } else if (strcmp(cmd[0], "cd") == 0) {
    if (cmd[1] == NULL) {
      chdir(getenv("HOME"));
    } else {
      chdir(cmd[1]);
    }
  } else if (strcmp(cmd[0], "lj") == 0) {
    lj();
  } else if (strcmp(cmd[0], "sj") == 0) {
    if (is_in_command_list(atoi(cmd[1]))) {
      sj(atoi(cmd[1]));
    }
  } else if (strcmp(cmd[0], "bg") == 0) {
    if (is_in_command_list(atoi(cmd[1]))) {
      bg(atoi(cmd[1]));
    }
  } else if (strcmp(cmd[0], "fg") == 0) {
    if (is_in_command_list(atoi(cmd[1]))) {
      fg(atoi(cmd[1]));
    }
  } else {
    fork_and_exec(cmd, is_backgrounded, pipe1, pipe2, l, i);

    if (is_backgrounded == false) {
      fg(id_command - 1);
    }
  }

}


// Gestion des signaux

void handle_sig(int sig) {
  int pid_retour;
  int status;
  if (sig == SIGCHLD) {

    do {
      pid_retour = waitpid(-1, & status, WNOHANG | WUNTRACED | WCONTINUED);
      if (pid_retour == -1) {
        ;
      } else if (pid_retour > 0) {
        if (WIFSTOPPED(status)) {
          //SUSPENSION
          printf("Le processus %d a été suspendu\n", pid_retour);
          foreground_active = false;
          commands_list[pid_to_id(pid_retour)].is_running = false;
        } else if (WIFCONTINUED(status)) {
          //>>> REPRISE
          printf("Le processus %d a été repris\n", pid_retour);
          commands_list[pid_to_id(pid_retour)].is_running = true;
        } else if (WIFEXITED(status)) {
          //>>> FIN NORMALE
          remove_command_from_list(pid_retour);
          foreground_active = false;
        } else if (WIFSIGNALED(status) && WTERMSIG(status)) {
          //>>> SIGNAL TUE 
          printf("Le processus %d a été tué\n", pid_retour);
          remove_command_from_list(pid_retour);
          foreground_active = false;
        }
      }
    } while (pid_retour > 0);
  }
}

void handle_stop(int sig) {
  if (sig == SIGTSTP) {
    for (int i = 0; i < id_command; i++) {
      if (commands_list[i].is_backgrounded == false) {
        foreground_active = false;
        kill(commands_list[i].pid, SIGSTOP);
        commands_list[i].is_running = false;
        commands_list[i].is_backgrounded = true;
      }
    }
  }
}

void handle_int(int sig) {

  bool command_found = false;
  if (sig == SIGINT) {
    for (int i = 0; i < id_command; i++) {
      if (commands_list[i].is_backgrounded == false) {
        kill(commands_list[i].pid, SIGKILL);
        command_found = true;
      }

    }

    if (command_found == false) {
      printf("Aucune commande en cours d'exécution, terminaison du shell\n");
      exit(0);
    }
  }

}



int main() {
  bool is_backgrounded = false;

  // Ajout des handlers

  signal(SIGTSTP, handle_stop);
  signal(SIGINT, handle_int);
  signal(SIGCHLD, handle_sig);
  do {

    printf("\n>>>");
    struct cmdline * l;
    l = readcmd();
    if (verify_command(l)) {

      int i = 0;
      int pipe1[2], pipe2[2];
      pipe(pipe1);

      while (l -> seq[i] != NULL) {

        char ** cmd = l -> seq[i];

        // On vérifie si la commande est en tâche de fond
        is_backgrounded = (l -> backgrounded != NULL);
        command_executer(cmd, is_backgrounded, pipe1, pipe2, l, i);

        i++;
      }

      if (i > 1) {
        // On ferme les pipes
        close(pipe1[0]);
        close(pipe1[1]);

      }

    } else {
      ;
    }

  } while (1);
  return EXIT_SUCCESS;
}