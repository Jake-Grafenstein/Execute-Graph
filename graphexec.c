/* login grafe014
 * date: 10/07/15
 * name: Jake Grafenstein, Kshitij Aranke
 * id: grafe014, arank002 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

//for 'status' variable:
#define INELIGIBLE 0
#define READY 1
#define RUNNING 2
#define FINISHED 3

//for constant values
#define MAX_NODES 50
#define MAX_CHAR 1024
#define MAX_CHILDREN 10
#define MAX_TOKENS 4

// For our redirects to input and output files
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

// For a boolean array of finished nodes
#define EXECUTED 1
#define UNEXECUTED 0

typedef struct node {
  int id; // corresponds to line number in graph text file
  char prog[MAX_CHAR]; // prog + arguments
  char input[MAX_CHAR]; // filename
  char output[MAX_CHAR]; // filename
  int children[MAX_CHILDREN]; // children IDs
  int num_children; // how many children this node has
  int status;
  int num_parents;
  int finished_parents;
  pid_t pid; // track it when it's running
} node_t;

int makeNodeArray(node_t *nodes, FILE * input_file, int *parents);
int makeargv(const char *s, const char *delimiters, char ***argvp);
void freemakeargv(char **argv);
void scheduleNodes(node_t *nodes, int lineNumber, int *parents, int *completed);
int execNode(node_t *node);

/*
 * The main() function first allocates memory on the heap for three arrays: nodes, (int) number of parents, (int) completed parents. It opens the file using fopen(), calls makeNodeArray() to create our data sctructure, and then calls scheduleNodes() to execute the instructions in the correct order. After that function returns, we free the memory allocated for the arrays and return.
 */
int main(int argc, char const *argv[]) {
  int nodeCount;
  FILE *input_file;
  node_t *nodes;
  int *number_parents;
  int *completed_parents;
  int i;
  // Allocate space for the Maximum number of nodes at the size of a node
  nodes = (node_t *) calloc(sizeof(node_t), MAX_NODES);
  number_parents = (int *) calloc(sizeof(int), MAX_NODES);
  completed_parents = (int *) calloc(sizeof(int), MAX_NODES);

  for (i = 0; i < MAX_NODES; i++) {
    number_parents[i] = 0;
    completed_parents[i] = UNEXECUTED;
  }
  // Get the filename
  char const *fname = argv[1];
  // Open the file
  input_file = fopen(fname, "r");
  if (!input_file) {
    perror("fopen failed to open the file");
  }
  if (input_file == 0) {
    perror("Cannot open file\n");
    exit(1);
  }
  // Make some nodes and put them in your array.
  nodeCount = makeNodeArray(nodes, input_file, number_parents);
  printf("This is my nodeCount: %d\n", nodeCount);

  fclose(input_file);
  // Execute the nodes in order.
  scheduleNodes(nodes, nodeCount, number_parents, completed_parents);
  // Release the memory so as to not have a memory leak
  free(nodes);
  free(number_parents);
  free(completed_parents);

  return 0;
}

/*
 * The makeNodeArray() function takes in three pointers (one for our node array, one for our input file, and one for our parent array) and using the fgets() function, separates the input file by line. We use the makeargv() function given in Robbins & Robbins to tokenize the line, and then match the arguments to the parameters of node we create in the array. We keep track of the number of parents in the parent array on the heap and return to main().
 */

int makeNodeArray(node_t *nodes, FILE *input_file, int *parents) {
  int line_number=0, arguments, i, j, childCounter;
  char line[MAX_CHAR];
  char noneChecker[MAX_CHAR];
  char **myargv;
  char **myChildArgV;
  int parameter = 0;
  while (fgets(line, sizeof(line), input_file) != NULL) {
    if (line[strlen(line) - 1] == '\n') {
			 line[strlen(line) - 1] = '\0'; // Remove extra lines from graph
		}
		if (strcmp(line, "\0") == 0) {
			 continue; // No blank lines are allowed.
    }
    // Tokenize the line
    arguments = makeargv(line, ":\n", &myargv);
    if (arguments != 4) {
      perror("Invalid number of input items");
    }

    nodes[line_number].id = line_number;
    strcpy(nodes[line_number].prog, myargv[0]);
    strcpy(noneChecker, myargv[1]);
    strcpy(nodes[line_number].input, myargv[2]);
    strcpy(nodes[line_number].output, myargv[3]);
    nodes[line_number].status = INELIGIBLE;
    // Does this node not have any children?
    if (!strcmp(myargv[1],"none")) {
      printf("We have found the node that has no children (i.e. end of file).\n");
    } else {
      // Tokenize the children using makeargv
      arguments = makeargv(myargv[1], " ", &myChildArgV);
      childCounter = 0;
      // Let's count the number of children in the Tokenized myChildArgV
      for (j = 0; j < MAX_CHILDREN; j++) {
        if (myChildArgV[j] == NULL) {
          break;
        }
        else {
          childCounter++;
        }
      }
      // Set the number of children
      nodes[line_number].num_children = childCounter;
      // Now, put those children in the child array
      for (j = 0; j < childCounter; j++) {
        nodes[line_number].children[j] = atoi(myChildArgV[j]);
        printf("%d\n", nodes[line_number].children[j]);
      }
    }
    line_number++;
  }
  // This keeps track of the number of parents each node has
  i = 0;
  j = 0;
  while (i < line_number) {
    while (j < nodes[i].num_children) {
      int childNode = nodes[i].children[j];
      parents[childNode]++;
      j++;
    }
    i++;
    j = 0;
  }
  printf("Exiting from makeNodeArray...\n");
  return line_number;
}

/*
 * Code in lines 38-75 taken from Unix Systems Programming by Robbins & Robbins, page 37
 * This function takes in pointer to an array of characters (a string), a delimiter (the character you want to use to separate the line into tokens) and a pointer to where you want the resulting tokens to be stored. The number of tokens created is returned (which given the parameters, we know should be 4).
 */
int makeargv(const char *s, const char *delimiters, char ***argvp) {
	 int error;
	 int i;
	 int numtokens;
	 const char *snew;
	 char *t;

	 if ((s == NULL) || (delimiters == NULL) || (argvp == NULL)) {
		 errno = EINVAL;
		 return -1;
	 }
	 *argvp = NULL;
	 snew = s + strspn(s, delimiters); /* snew is real start of string */
	 if ((t = malloc(strlen(snew) + 1)) == NULL)
		 return -1;
	 strcpy(t, snew);
	 numtokens = 0;
	 if (strtok(t, delimiters) != NULL) /* count the number of tokens in s */
		 for (numtokens = 1; strtok(NULL, delimiters) != NULL; numtokens++)
			 ;

	 /* create argument array for ptrs to the tokens */
	 if ((*argvp = malloc((numtokens + 1) * sizeof(char *))) == NULL) {
     error = errno;
		 free(t);
		 errno = error;
		 return -1;
	 }
	 /* insert pointers to tokens into the argument array */
	 if (numtokens == 0)
		 free(t);
	 else {
		 strcpy(t, snew);
		 **argvp = strtok(t, delimiters);
		 for (i = 1; i < numtokens; i++)
			 *((*argvp) + i) = strtok(NULL, delimiters);
	 }
	 *((*argvp) + numtokens) = NULL; /* put in final NULL pointer */
	 return numtokens;
 }

/*
 * Code from lines 78-86 taken from Unix Systems Programming by Robbins & Robbins, page 38
 * The freemakeargv() function frees the memory that was allocated during the makeargv function and is a fairly straightfoward function.
 */
void freemakeargv(char **argv) { //AKA the poor-man's makeargv
  if (argv == NULL) {
    return;
  }
  if (*argv != NULL) {
    free(*argv);
  }
  free(argv);
}

/*
 * The scheduleNodes() function takes in four parameters (a pointer to our node array, an int of the number of nodes, a pointer to the parent array and a pointer to the completed node array) and first initializes the first node of the array to be ready to run. Then, within a while loop that compares the completed nodes to the number of nodes, we loop over all of our nodes to find those that are ready to run. For a node to be ready to run, all of its parents must completed execution. When a node is ready to run, we fork() the process, creating a child process where the node can be executed. The parent waits for the child process to be finished executing, sets its own children to be ready to run, and continues through the loop. The function also checks if there is a cycle in the graph by using the completed node array to check if any node's children have already been executed.
 */
void scheduleNodes(node_t *nodes, int lineNumber, int *parents, int *completed) {
  int waitStatus;
  pid_t childpid;
  int i, j, k;
  int executedNodes = 0;
  int executedList[lineNumber];
  node_t myNode, childNode;
  // Set head node to READY
  nodes[0].status = READY;
  // Initialize executedList to UNEXECUTED;
  for (i = 0; i < lineNumber; i++) {
    executedList[i] = UNEXECUTED;
  }
  printf("Entering our ScheduleNodes...\n");
  // This while loop checks to see if we've executed all the nodes
  while (executedNodes < lineNumber) {
    // This for loop loops through all of our nodes
    for (i = 0; i < lineNumber; i++) {
      myNode = nodes[i];
      printf("What's my node's number?: %d\n", i);

      if (myNode.status == INELIGIBLE) {
      	if (parents[myNode.id] == completed[myNode.id]) {
          myNode.status = READY;
          printf("Node %d is ready\n", i);
		    }
      }
      if (myNode.status == READY) { //Can we execute on the node?
      	printf("Forking node %d\n", i);
      	childpid = fork();

      	if (childpid == -1) { // unable to fork
      		perror("Could not fork");
      		exit(1);
      	} else if (childpid == 0) { // could fork successfully, child is next in line
      		execNode(&myNode);
          break; // No longer need the child process, break out of the loop.
      	} else { // If we are the parent process, we need to set our status to running until the child process finishes
      		myNode.status = RUNNING;
      		myNode.pid = childpid;
      	}
      }

      if (myNode.status == RUNNING) {
    		// Check to see if the child process has finished
        printf("I'm waiting on my child...\n");
    		waitStatus = wait(0);
    		if (waitStatus == -1) {
    		 perror("Failed to get status");
    		 exit(1);
    		}
    		else {
    		 myNode.status = FINISHED;
         executedList[myNode.id] = EXECUTED;
    		 executedNodes++;
    		 for (j = 0; j < nodes[i].num_children; j++) {
           childNode = nodes[myNode.children[j]];
           completed[myNode.children[j]]++;
           // Check for cycles in the graph, do any of the children point back to the a completed process?
           for (k=0; k < childNode.num_children; k++) {
             if (executedList[childNode.children[k]] == EXECUTED) {
               perror("Cycle found in graph.");
               exit(1);
             }
           }
    		 }
    		 printf("Node %d is complete\n", i);
    		}
      }
    }
  }
}
/*
 * The execNode() function takes in a pointer to the node we are trying to execute. First we redirect the input and output files as given to us in the assignment. Then we use the makeargv() function to separate the command line arguments into tokens, and call execvp() to execute the arguments. Lastly, freemakeargv is called to free the allocated memory.
 */
int execNode(node_t *node) {
  char **myArgv;
  char *delim = " ";
  int numTokens;
  int i;
  // code from line 75-93 comes from redirect.c which was given with the assignment
    // Redirects the input file
    printf("Executing Node: %d\n", node -> id);
    int input_file = open(node -> input, O_RDONLY, 0666);
    if (input_file < 0){
      perror("Error opening input file in child after fork! Exiting.");
      exit(0);
    } else {
      dup2(input_file, STDIN_FILENO);
      close(input_file);
    }

    // Redirects the output file
    int output_file = open(node -> output, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (output_file < 0) {
      perror("Error opening output file in child after fork! Exiting.");
      exit(0);
    } else {
      dup2(output_file, STDOUT_FILENO);
      close(output_file);
    }
  numTokens = makeargv(node -> prog, delim, &myArgv); // Let's create a makeargv with the program name
  execvp(myArgv[0], myArgv); //Now, execute it.
  freemakeargv(myArgv); // Prevent a memory leak.
  return 1;
}
