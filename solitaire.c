//------------------------------------------------------------------------------
// Authors: Paul Kodolitsch
//          Martin Piberger
//------------------------------------------------------------------------------
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define NUMBER_OF_STACKS 7
#define NUMBER_OF_GAMESTACKS 4
#define NUMBER_OF_CARDFACES 2
#define BOARD_SIZE 16
#define MAX_COMMAND_ARG 5
#define QUIT_GAME_ERRORS -4
#define DRAWSTACK 0

// Memory allocation
#define SIZE 20
#define TWO 2

#define WHITESPACE 32
#define BLACK_KING 24

#define DEPOSIT_STACK_1 5
#define DEPOSIT_STACK_2 6

// Commands
#define COMMAND_TYPE 0
#define COMMAND_FIRST_ARG 1
#define MOVE_CARD_COLOR 1
#define MOVE_CARD_RANK 2
#define MOVE_TO 3
#define MOVE_TARGET_STACK 4

// Struct defines values for cards and are used for creating a
// doubly linked list
typedef struct Node
{
  int card_value_;
  bool is_faced_up_;
  struct Node* next_;
  struct Node* prev_;
}Node;

typedef struct _Doubly_Linked_List_
{
  Node* head_;
  Node* tail_;
}Doubly_Linked_List;

// Return values of the program
typedef enum _ReturnValue_
{
  MOVED = 2,
  EXIT_GAME = 1,
  EVERYTHING_OK = 0,
  INVALID_MOVE_COMMAND = -1,
  INVALID_COMMAND = -2,
  INVALID_CARD = -3,
  INVALID_ARG_COUNT = -4,
  INVALID_FILE = -5,
  OUT_OF_MEMORY = -6,
  UNIDENTIFIED_ERROR = -7
} ReturnValue;

// Forward declarations
ReturnValue printErrorMessage(ReturnValue return_value);
void append(Doubly_Linked_List* list_ref, int card, bool isDrawstack);
void push(Doubly_Linked_List* list_ref, int card);
int pop(Doubly_Linked_List* list_ref);
void rotateDrawstack(Doubly_Linked_List* drawstack);
void arrangeCards(Doubly_Linked_List stacks[]);
Node* newNode(int card_value);
ReturnValue readConfig(FILE* file, Doubly_Linked_List* draw_stack);
void printGame(Doubly_Linked_List stacks[]);
void printCard(Node* card);
ReturnValue readInput(char** user_input, int* size);
ReturnValue handleCommand(Doubly_Linked_List stacks[], char* user_input);
ReturnValue printHelp(char* command[]);
ReturnValue moveCommand(Doubly_Linked_List stacks[], char* command[]);
bool checkMove(Doubly_Linked_List stacks[], int target_card, int target_stack,
   int* target_card_index, int* target_card_stack);
bool searchCard(Doubly_Linked_List stacks[], int target_card,
   int* target_card_index, int* target_card_stack);
bool twoCardsInOrder(int bottom_card, int top_card, int target_stack);
bool checkOrder(Doubly_Linked_List stack, int target_card_index,
   int target_stack);
void deleteStacks(Doubly_Linked_List stacks[]);
ReturnValue strToCard(char* color, char* rank, int* card);
ReturnValue splitString(char* string, char* arguments[]);
ReturnValue move(Doubly_Linked_List stacks[], int target_stack,
  int target_card_index, int target_card_stack);

//-----------------------------------------------------------------------------
///
/// The main program
/// Checks for file and starts the gameloop
///
/// @param argc number of arguments
/// @param argv program arguments
///
/// @return value of ReturnValue which defines type of error
//
int main(int argc, char* argv[]) {

  if (argc != 2)
  {
  	return printErrorMessage(INVALID_ARG_COUNT);
  }

  Doubly_Linked_List stacks[NUMBER_OF_STACKS] = { NULL };

  FILE* file = fopen(argv[1], "r");
  if (file == NULL)
  {
    return printErrorMessage(INVALID_FILE);
  }

  ReturnValue return_value = readConfig(file, &stacks[0]);
  if(return_value != EVERYTHING_OK)
  {
    deleteStacks(stacks);
    return printErrorMessage(return_value);
  }
  fclose(file);

  arrangeCards(stacks);

  printGame(stacks);
  char* user_input = (char*) malloc(SIZE);
  int size = SIZE;
  if (user_input == NULL)
  {
    return printErrorMessage(OUT_OF_MEMORY);
  }

  while (true) // Starts gameloop
  {
    return_value = readInput(&user_input, &size);
    if (return_value != EVERYTHING_OK)
    {
      printErrorMessage(return_value);
      break;
    }
    return_value = handleCommand(stacks, user_input);

    if (return_value < EVERYTHING_OK) //error values are negative
    {
      printErrorMessage(return_value);
      // quit game if error is a quitgameerror
      if (return_value <= QUIT_GAME_ERRORS) 
      {
        break;
      }
    }

    if (return_value == MOVED) // A valid command has been executed
    {
      printGame(stacks);

      if ((stacks[DEPOSIT_STACK_1].tail_ != NULL && stacks[DEPOSIT_STACK_2].tail_
         != NULL) && stacks[DEPOSIT_STACK_1].tail_->card_value_ +
         stacks[DEPOSIT_STACK_2].tail_->card_value_ == 49)
      {
        printErrorMessage(return_value);
        break;
      }
    }
    if (return_value == EXIT_GAME)
    {
      break;
    }
  }
  
  free(user_input);
  user_input = NULL;
  deleteStacks(stacks);
  return EVERYTHING_OK;
}

//-----------------------------------------------------------------------------
///
/// Rotate the drawstack
///
/// @param drawstack struct of the doubly linked list
///
//
void rotateDrawstack(Doubly_Linked_List* drawstack)
{
  push(drawstack, pop(drawstack));
}

//-----------------------------------------------------------------------------
///
/// Deletes stacks
///
/// @param stacks array struct of the doubly linked list
///
//
void deleteStacks(Doubly_Linked_List stacks[])
{
  Node* current_node;
  Node* next_node;
  for (int index = 0 ; index < NUMBER_OF_STACKS ; index++)
  {
    current_node = stacks[index].head_;
    while(current_node)
    {
      next_node = current_node->next_;
      free(current_node);
      current_node = next_node;
    }
    stacks[index].head_ = NULL;
    stacks[index].tail_ = NULL;
  }
}

//-----------------------------------------------------------------------------
///
/// Checks order of two cards
///
/// @param bottom_card describes card position
/// @param top_card describes card position
/// @param target_stack defines the different stacks
///
/// @return boolean data type true or false
//
bool twoCardsInOrder(int bottom_card, int top_card, int target_stack)
{
  // Target_stack is one of the game stacks and color
  // Target_stack is one of the deposit stacks and topcard doesn't fit
  if ((target_stack <= NUMBER_OF_GAMESTACKS &&
    ((bottom_card % TWO == top_card % TWO) ||
    bottom_card / TWO <= top_card / TWO)) || 
    (target_stack > NUMBER_OF_GAMESTACKS &&
    ((bottom_card % TWO != top_card % TWO) ||
    (top_card - bottom_card != TWO))))
  {
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
///
/// Checks order of cards on top of the target card
///
/// @param stack struct of the doubly linked list
/// @param target_card_index integer number of a card
/// @param target_stack defines the different stacks
///
/// @return boolean data type true or false
//
bool checkOrder(Doubly_Linked_List stack, int target_card_index,
  int target_stack)
{
  Node* card_pointer = stack.head_;
  // Get to target element
  for (int index = 0 ; index < target_card_index ; index++)
  {
    card_pointer = card_pointer->next_;
  }
  while(card_pointer->next_)
  {
    if (!twoCardsInOrder(card_pointer->card_value_,
      card_pointer->next_->card_value_, target_stack))
    {
      return false;
    }
    card_pointer = card_pointer->next_;
  }
  return true;
}

//-----------------------------------------------------------------------------
///
/// Searchs through the doubly linked list for a specific card
///
/// @param stacks array struct of the doubly linked list
/// @param target_card specific card
/// @param target_card_index pointer to locate the cards position
/// @param target_card_stack pointer to locate the cards position
///
/// @return boolean data type true or false
//
bool searchCard(Doubly_Linked_List stacks[], int target_card,
   int* target_card_index, int* target_card_stack)
{
  Node* current_card;
  for (int col = 0 ; col < NUMBER_OF_STACKS ; col++)
  {
    current_card = stacks[col].head_;

    for (int row = 0 ; row < BOARD_SIZE && current_card != NULL; row++)
    {
      if (current_card->card_value_ == target_card && current_card->is_faced_up_)
      {
        *target_card_index = row;
        *target_card_stack = col;
        return true;
      }
      if(current_card->next_ != NULL)
      {
        current_card = current_card->next_;
      }
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
///
/// Checks if move command is valid or invalid
///
/// @param stacks array struct of the doubly linked list
/// @param target_card specific card
/// @param target_stack specific stack
/// @param target_card_index pointer to locate the cards position
/// @param target_card_stack pointer to locate the cards position
///
/// @return boolean data type true or false
//
bool checkMove(Doubly_Linked_List stacks[], int target_card,
  int target_stack, int* target_card_index, int* target_card_stack)
{
  bool card_found = searchCard(stacks, target_card, target_card_index,
    target_card_stack);
  if (!card_found)
  {
    return false;
  }
  if (*target_card_stack > NUMBER_OF_GAMESTACKS) 
  {
    return false;
  }

  bool in_order = checkOrder(stacks[*target_card_stack],
    *target_card_index, target_stack);

  if (in_order)
  {
    if (stacks[target_stack].tail_ == NULL)
    {
      if (target_stack <= NUMBER_OF_GAMESTACKS) // Is target_stack a Gamestack
      {
        return target_card >= BLACK_KING ? true : false; // BK or RK
      }
      return target_card < NUMBER_OF_CARDFACES ? true : false; // BA or RA
    }
    else if ((twoCardsInOrder(stacks[target_stack].tail_->card_value_,
      target_card, target_stack) || target_stack == *target_card_stack))
    {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
///
/// Determines a card move on the gameboard and locates the cards position
/// by counting rows and columns
///
/// @param stacks array struct of the doubly linked list
/// @param target_card_index defines the location in terms of rows
/// @param target_card_stack defines the location in terms of columns
///
/// @return value to evaluate the occurrence of an error
//
ReturnValue move(Doubly_Linked_List stacks[], int target_stack,
   int target_card_index, int target_card_stack)
{
  Node* target_card = stacks[target_card_stack].head_;
  Node* keep_tail = stacks[target_card_stack].tail_;

  for (int index = 0 ; index < target_card_index ; index++)
  {
    if (target_card->next_ == NULL)
    {
      return UNIDENTIFIED_ERROR;
    }
    target_card = target_card->next_;
  }

  stacks[target_card_stack].tail_ = target_card->prev_;

  if (target_card->prev_ == NULL)
  {
    stacks[target_card_stack].head_ = NULL;
  }
  else
  {
    stacks[target_card_stack].tail_->next_ = NULL;
    stacks[target_card_stack].tail_->is_faced_up_ = true;
  }

  if (stacks[target_stack].tail_ == NULL)
  {
    stacks[target_stack].head_ = target_card;
    stacks[target_stack].head_->prev_ = NULL;
    stacks[target_stack].tail_ = keep_tail;
  }
  else
  {
    stacks[target_stack].tail_->next_ = target_card;
    target_card->prev_ = stacks[target_stack].tail_;
    stacks[target_stack].tail_ = keep_tail;
  }
  return MOVED;
}

//-----------------------------------------------------------------------------
///
/// Examines a move command and calls various functions to execute the command
///
/// @param stacks array struct of the doubly linked list
/// @param command array pointer defines the command given by an user
///
/// @return value to evaluate the occurrence of an error or determine a
/// function call
//
ReturnValue moveCommand(Doubly_Linked_List stacks[], char* command[])
{
  char* to = "TO";
  if (command[MOVE_TARGET_STACK] == NULL || strcmp(command[MOVE_TO], to) != 0)
  {
    return INVALID_COMMAND;
  }

  int target_card = 0;
  int target_card_index;
  int target_card_stack;
  int target_stack = strtol(command[MOVE_TARGET_STACK], NULL, 10);

  if (target_stack < 1 || NUMBER_OF_STACKS < target_stack)
  {
    return INVALID_COMMAND;
  }
  ReturnValue return_value = strToCard(command[MOVE_CARD_COLOR],
    command[MOVE_CARD_RANK], &target_card);
  if (return_value != EVERYTHING_OK) 
  {
    return return_value;
  }

  bool move_valid = checkMove(stacks, target_card, target_stack,
    &target_card_index, &target_card_stack);

  if (!move_valid)
  {
    return INVALID_MOVE_COMMAND;
  }

  if (target_card_stack != target_stack)
  {
    return move(stacks, target_stack, target_card_index, target_card_stack);
  }
  return MOVED;
}

//-----------------------------------------------------------------------------
///
/// Compares the user input to various command possibilities
///
/// @param stacks array struct of the doubly linked list
/// @param user_input pointer to an allocated user input string
///
/// @return value to evaluate the occurrence of an error or determine a
/// function call
//
ReturnValue handleCommand(Doubly_Linked_List stacks[], char* user_input)
{
  char* help = "HELP";
  char* exit_game = "EXIT";
  char* move = "MOVE";
  char* next = "NEXT";

  char* command[MAX_COMMAND_ARG];
  if (splitString(user_input, command) != EVERYTHING_OK) 
  {
    return INVALID_COMMAND;
  }
  if (command[COMMAND_TYPE] == NULL)
  {
    return INVALID_COMMAND;
  }
  if (strcmp(command[COMMAND_TYPE], move) == 0)
  {
    return moveCommand(stacks, command);
  }
  else if (strcmp(command[COMMAND_TYPE], next) == 0)
  {
    rotateDrawstack(&stacks[DRAWSTACK]);
    return MOVED;
  }
  else if (strcmp(command[COMMAND_TYPE], help) == 0)
  {
    return printHelp(command);
  }
  else if (strcmp(command[COMMAND_TYPE], exit_game) == 0)
  {
    return EXIT_GAME;
  }
  return INVALID_COMMAND;
}

//-----------------------------------------------------------------------------
///
/// Splits a string in a number of phrases seperated by whitespaces
///
/// @param string pointer to an allocated user input string
/// @param arguments array pointer to a command
///
/// @return value to evaluate the occurrence of an error
//
ReturnValue splitString(char* string, char* arguments[])
{
  arguments[0] = strtok(string, " ");
  for (int index = 1; index < MAX_COMMAND_ARG; index++)
  {
    arguments[index] = strtok(NULL, " ");
  }
  if (strtok(NULL, " ") != NULL)
  {
    return INVALID_COMMAND; // more arguments then allowed
  }
  return EVERYTHING_OK;
}

//-----------------------------------------------------------------------------
///
/// Reads input and filters unnecessary whitespaces. Reallocates memory if
/// more is needed
///
/// @param user_input pointer to an allocated user input string
///
/// @return value to evaluate the occurrence of an error
//
ReturnValue readInput(char** user_input, int* size)
{
  char text = 0;
  int index = 0;
  int whitespace_flag = 0;

  printf("esp> ");
  if (user_input == NULL)
  {
    return OUT_OF_MEMORY;
  }
  while ((text = toupper(getchar())) != '\n' && text != EOF)
  {
    if (index < *size)
    {
      if (text == WHITESPACE)
      {
        if (whitespace_flag)
        {
          continue;
        }
        whitespace_flag = 1;
        (*user_input)[index] = text;
        index++;
      }
      else
      {
        (*user_input)[index] = text;
        index++;
        whitespace_flag = 0;
      }
    }
    else
    {
      *size *= TWO;
      char* tmp = *user_input;
      *user_input = (char*) realloc(*user_input, *size);
      if (*user_input == NULL)
      {
        free(tmp);
        return OUT_OF_MEMORY;
      }
      (*user_input)[index] = text;
      index++;
    }
  }
  (*user_input)[index] = '\0';
  return EVERYTHING_OK;
}

//-----------------------------------------------------------------------------
///
/// Prints possible commands on the user interface
///
/// @param command array pointer defines the command given by an user
///
/// @return value to evaluate the occurrence of an error
//
ReturnValue printHelp(char* command[])
{
  if (command[COMMAND_FIRST_ARG] == NULL)
  {
    printf("possible command:\n");
    printf(" - move <color> <value> to <stacknumber>\n");
    printf(" - help\n");
    printf(" - exit\n");
    return EVERYTHING_OK;
  }
  return INVALID_COMMAND;
}

//-----------------------------------------------------------------------------
///
/// Follows the doubly linked list to specific fields and calls for further
/// functions
///
/// @param stacks array struct of the doubly linked list
///
//
void arrangeCards(Doubly_Linked_List stacks[])
{
  for (int row = 1 ; row < NUMBER_OF_GAMESTACKS + 1 ; row++)
  {
    for (int col = row ; col < NUMBER_OF_GAMESTACKS + 1 ; col++)
    {
      append(&stacks[col], pop(&stacks[0]),false);
    }
  }
}

//-----------------------------------------------------------------------------
///
/// Prints the gameboard on the user interface
///
/// @param stacks array struct of the doubly linked list
///
//
void printGame(Doubly_Linked_List stacks[])
{
  printf("0   | 1   | 2   | 3   | 4   | DEP | DEP\n");
  printf("---------------------------------------\n");
  Node* head[NUMBER_OF_STACKS];
  for (int index = 0; index < NUMBER_OF_STACKS; index++)
  {
    head[index] = stacks[index].head_;
  }
  for (int row = 0; row < BOARD_SIZE; row++) //print a line of the game
  {
    for (int col = 0; col < NUMBER_OF_STACKS; col++)
    {
      if (col != 0)
			{
        printf(" ");
      }
      if (head[col])
      {
        printCard(head[col]);
        head[col] = head[col]->next_;
      }
      else
      {
        printf("   ");
      }
      if (col != NUMBER_OF_STACKS-1)
      {
        printf(" |");
      }
    }
    printf("\n");
  }
}

//-----------------------------------------------------------------------------
///
/// Prints a card on the gameboard
///
/// @param card struct saves the value of a card
///
//
void printCard(Node* card)
{
  char* ranks[] = { "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J",
   "Q", "K" };
  char color[] = { 'B', 'R' };

  if (card->is_faced_up_)
  {
    printf("%c%-2s", color[card->card_value_ % 2], ranks[card->card_value_ / 2]);
  }
  else
  {
    printf("X  ");
  }
}

//-----------------------------------------------------------------------------
///
/// Prints message which describes the return value
///
/// @param return_value type of main return value
///
/// @return parameter return_value
//
ReturnValue printErrorMessage(ReturnValue return_value)
{
  switch (return_value)
  {
  case INVALID_CARD:
    printf("[INFO] Invalid card!\n");
    break;
  case INVALID_COMMAND:
    printf("[INFO] Invalid command!\n");
    break;
  case INVALID_MOVE_COMMAND:
    printf("[INFO] Invalid move command!\n");
    break;
  case INVALID_ARG_COUNT:
    printf("[ERR] Usage: ./ass3 [file-name]\n");
    return_value = 1;
    break;
  case INVALID_FILE:
    printf("[ERR] Invalid file!\n");
    return_value = 3;
    break;
  case OUT_OF_MEMORY:
    printf("[ERR] Out of memory!\n");
    return_value = 2;
    break;
  case UNIDENTIFIED_ERROR:
    printf("[ERR] Unidentified error!\n");
    break;
  case MOVED:
    //left blank intentionally
    break;
  case EXIT_GAME:
    //left blank intentionally
    break;
  case EVERYTHING_OK:
    //left blank intentionally
    break;
  }
  return return_value;
}

//-----------------------------------------------------------------------------
///
/// Transform a string to an comparable integer
///
/// @param color pointer defines the color of a card
/// @param rank pointer defines the rank of a card
/// @param card pointer defines the integer value of a card
///
/// @return value to evaluate the occurrence of an error
//
ReturnValue strToCard(char* color, char* rank, int* card)
{
  char* ranks[] = { "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J",
   "Q", "K" };

  if (strcmp(color, "BLACK") != 0 && strcmp(color, "RED") != 0)
  {
    return INVALID_COMMAND;
  }

  for (int index = 0; index < 13; index++)
  {
    if (strcmp(ranks[index], rank) == 0)
    {
      *card = (index * 2) + (strcmp(color, "BLACK") == 0 ? 0 : 1);
      break;
    }
  }
  return EVERYTHING_OK;
}

//-----------------------------------------------------------------------------
///
/// Reads cards from a configuration file
///
/// @param file pointer to file to read from
/// @param card pointer defines the integer value of a card
///
/// @return value to evaluate the occurrence of an error or execute a function
//
ReturnValue readCard(FILE* file, int* card)
{
  char color[6];
  char rank[3];

  if (fscanf(file, " %s %s", color, rank) != 2)
  {
    return INVALID_FILE;
  }
  return strToCard(color, rank, card);
}

//-----------------------------------------------------------------------------
///
/// Checks input of a configuration file
///
/// @param file pointer to file to read from
/// @param draw_stack struct of the doubly linked list
///
/// @return value to evaluate the occurrence of an error
//
ReturnValue readConfig(FILE* file, Doubly_Linked_List* draw_stack)
{
  int checkArray[26] = { 0 };
  ReturnValue read_error;
  int card;

  for (int index = 0; index < 26; index++)
  {
    if ((read_error = readCard(file, &card)) != EVERYTHING_OK)
    {
      return INVALID_FILE;
    }
    if (checkArray[card] == 0)
    {
      //checkArray[card]++;
      append(draw_stack, card++, true);
    }
    else
    {
    printf("NICE");
      return INVALID_FILE;
    }
  }

  if (readCard(file, &card) == EVERYTHING_OK)
  {
    return INVALID_FILE;
  }
  return EVERYTHING_OK;
}

//-----------------------------------------------------------------------------
///
/// Pops last element of list, returns its card_ value and frees it 
///
/// @param list_ref pointer to doubly linked list
///
/// @return or removes tail node
//
int pop(Doubly_Linked_List* list_ref)
{
  if (list_ref->tail_ == NULL)
  {
    return 0;
  }
  int card_value = list_ref->tail_->card_value_;
  Node* prev = list_ref->tail_->prev_;
  free(list_ref->tail_);
  list_ref->tail_ = prev;

  if (list_ref->tail_ != NULL)
  {
    list_ref->tail_->next_ = NULL;
    list_ref->tail_->is_faced_up_ = true;
  }
  else
  {
    list_ref->head_ = NULL;
  }
  return card_value;
}

//-----------------------------------------------------------------------------
///
/// Add a card to back of the List
///
/// @param list_ref struct of the doubly linked list
/// @param card defines the card to add
///
//
void append(Doubly_Linked_List* list_ref, int card, bool isDrawstack) {
  Node* node = newNode(card);
  if (list_ref->head_ == NULL)
  {
    node->is_faced_up_ = true;
    list_ref->head_ = node;
    list_ref->tail_ = node;
    return;
  }
  list_ref->tail_->next_ = node;
  node->prev_ = list_ref->tail_;
  list_ref->tail_ = node;
  list_ref->tail_->is_faced_up_ = true;
  if (list_ref->tail_->prev_ != NULL && isDrawstack)
  {
    list_ref->tail_->prev_->is_faced_up_ = false;
  }
}

//-----------------------------------------------------------------------------
///
/// Add a card to front of the List
///
/// @param list_ref struct of the doubly linked list
/// @param card defines the card to add
///
//
void push(Doubly_Linked_List* list_ref, int card)
{
  Node* node = newNode(card);
  if (list_ref->head_ == NULL)
  {
    list_ref->head_ = node;
    list_ref->tail_ = node;
    return;
  }
  list_ref->head_->prev_ = node;
  node->next_ = list_ref->head_;
  list_ref->head_ = node;
}

//-----------------------------------------------------------------------------
///
/// Creates new node
///
/// @param card defines the card to add
///
/// @return struct node that was created
//
Node* newNode(int card)
{
  Node* node = (Node*)malloc(sizeof(Node));
  node->card_value_ = card;
  node->is_faced_up_ = false;
  node->next_ = NULL;
  node->prev_ = NULL;
  return node;
}

