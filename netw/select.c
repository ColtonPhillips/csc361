/*-------------------------------------------------------------------------
		An Example for Select() Function in C
 Reference: http://psy.swansea.ac.uk/staff/Carter/projects/c_keywait.html
		   How to Wait for a Key Press in C

The fds is a file descriptor set -- the select function checks if the specified fds are ready for reading or writing.
The file descriptor in this case is specified to be stdin for the keyboard. The first NULL argument is checked for writing, but we are only interested in reading (input) files, so we can pass in a null. By running the ./select binary file, the code simultaneously read the keyboard input and check the timeout deadline (2 sec). So, all letters that have been entered by the user before reaching the timeout will be displayed on the screen. Select() monitors both of them and runs eah one that comes first. When the codereaches timeout deadline, the program terminates.
-------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int
main( void )
{
   int select_result;
   fd_set read_fds;
   struct timeval timeout;
   
   /* FD_ZERO() clears out the called socks, so that it doesn't contain any file descriptors. */
   FD_ZERO( &read_fds );
   /* FD_SET() adds the file descriptor "read_fds" to the fd_set, so that select() will return the character if a key is pressed */
   FD_SET( STDIN_FILENO, &read_fds );
   timeout.tv_sec = 2;
   timeout.tv_usec = 0;
   select_result = select( 1, &read_fds, NULL, NULL, &timeout );

   switch (select_result) {
      case -1:
      printf( "Error\n" );
      break;
      case 0:
      printf( "Timeout\n" );
      break;
      default:
      /* Select returns a one if a key is pressed */
      printf( "Number = %d\n", select_result );
   }

   return;
}
