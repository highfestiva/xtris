# xtris -- a multi-player version of Tetris for the X Window system

			
Roger Espel Llima <roger.espel.llima@pobox.com>
(ported to win32 by Vedran Vidovic <vvidovic@inet.hr>)
("Offline movements" by Jonas Byström <highfestiva@gmail.com> 2016.)

xtris is a version of the classical game of Tetris, for any number of
players, for the X Window system.

xtris is a true client/server game (as opposed to a centralized game
managing multiple displays), which makes it particularily responsive and
bandwith-effective.

Although it has a somewhat motif-ish look and feel, xtris doesn't use
any toolkit libraries, which makes it small and easy to compile (and
kind of a mess to program).

As an extra bonus, xtris includes a small chat system where players can
type messages at each other during the game.

And, the final reason for choosing xtris: it uses exactly the same brick
definitions as the *original* tetris (the one written in Turbo Pascal 3
by the two Russian guys), so when you rotate your brick and drop it, it
will fall just where you expect it.


Multi-player games are managed by the xtris server, xtserv; they are
synchronous: all users play at the same level, start at the same time,
and see each other's games.  The game stops when only one player remains.  

xtris keeps track of the number of lines each player has filled in the
current game, and of the number of games each player has won.

Each player has a name, initially set to his login name, or the contents
of the environment variables XTRISNAME or NETTRISNAME.  

There are two modes for the game:

* normal

   The game starts with an empty pit.  Whenever one user fills two or
   more rows at the same time, these lines appear at the bottom of some
   other user's screen.

   At the beginning of the game, each player is set to send his lines to
   the next player, in the order of connection to the server.  After
   sending some lines, the server chooses a next victim for the player,
   sequentially.

* fun 

   The game starts with some garbage at the bottom and the side of the
   pit.  Whenever one player fills a line, his victim gets an extra gray
   square at the highest point of his game.  Filling two lines
   simultaneously sends one line; filling three sends three, and filling
   four sends five.  


You can add automatic players, or "bots", to the game, and play against
them.  Each bot is a running copy of the program 'xtbot', connected to
the same server.

Whenever there is more than one player, players can type messages to
each other, which appear on a special message area; players who have
already lost can chat this way while the game continues.


## installation

Unzip exe files and 'startXtris.bat' file into same directory. Edit
startXtris.bat in any text editing program to redefine keys used for playing.
Use 'startXtris.bat' to start game.


## playing
   
To start a game, click on the 'start' button, after choosing your
initial level and mode.  

Unless started with the -nospeedup option, the server will increase the
level whenever the number of lines is greater than 10 times the current
level.

xtris recognizes these keys (some of them are shortcuts for the various
buttons):

	s       --    start a game
	q       --    quit the game
	p       --    pause/continue the game
	n       --    change the game mode between normal and fun
	b       --    start a bot (automatic player)
	t       --    open/close the message area
	c       --    clear the message area
	v       --    toggle verbose mode
	z       --    set the game counters to 0
	0 - 9   --    set the level
	Tab     --    warp the mouse pointer between the game area,
		      the nick entry area, and the message area
	Return  --    toggle seeing the next brick

in addition to the game keys:

        J       --    move left all the way
	j       --    move one step to the left
	k       --    counter-clock-wise rotation
	l       --    move one step to the right
	L       --    move right all the way
	,       --    drop the brick one step
	space   --    drop the brick all the way

Alternatively, you can use the arrow keys to move the piece around one
step, and the shifted arrow keys to move it all the way in each direction.
The Up arrow key rotates.


The buttons on the 'xtris' screen are:

        start   --    start a game (resets the game for all players)
        quit    --    exit, disconnecting from the server
	pause   --    pause/continue the game
        bot     --    start a bot (automatic player) on the same server
        0 - 9   --    set the level
        normal  --    toggle normal/fun game mode

	text    --    show/hide the message area (only when there are
		      at least two players)
	zero	--    set the game counters to 0
	verbose --    toggle displaying of server messages about the
		      game in the message area.
	clear   --    clear the message area.


Clicking on the area where the Next brick is displayed also toggles
displaying the Next.

At any time, you can change your name by moving the mouse pointer on it
(at the bottom of your screen) and typing.  The only recognized editing
keys are ^U to clear the line, Backspace, and Return when you're done.

You can enter accented iso-latin-1 characters by holding down the Meta
key (or whatever is set to mod1 in your X11 keyboard mapping).

Clicking on a bot's name, at the bottom of the screen, will remove it
from the game.


The message area is a zone under the tetris pits where users can chat by
typing messages at each other.  It is not displayed by default; to
activate it, click on the "msgs" button, which appears as soon as there
are two or more users. 

When you put your mouse pointer in the message area, the text you type
goes to the input line at the very bottom of the window.  When you press
Return, your line gets sent to all other players.

If you hold down the Shift key when pressing Return in the message area,
the mouse pointer gets warped back to the main game. This lets you type
in a message quickly during the game: press Tab to warp to the message
area, and end your message with Shift-Return to warp back.


## xtris

You start xtris with:

  xtris [ -display machine:0 ] [ -help ] [ -k keys ] [ -s ]  [ -bw ] 
	[ -n nick ] [ -big ] [ -flashy ] [ -norestart ] [ -nospeedup ] 
	[ server.name [ port ] ]

and the options mean:

     -bw  
	  With this option, the game will be  drawn  in  black  &
          white even on a color display.

     -big
	  Makes xtris use larger graphics.

     -display
          Specify the X display where xtris will open a window to
          play  the  game.   By  default,  xtris  connects to the
          display specified in the environment variable DISPLAY.

     -flashy
          Selects an alternate, flashier set of  colors  for  the
          tetris bricks.

     -help
          Prints out a summary of the command-line options  xtris
          recognizes.

     -k   Redefine the keys used by xtris. The argument must be a
          6-letter  string  with  the  keys  to  move to the left
          border, to move one position to the left, to rotate, to
          move  one  position  to the right, to move to the right
          border  and  to  drop  the  brick,  respectively.   The
          default keys are 'JjklL '.

     -n nick
          Sets  the nickname  for  the  player.  The max nickname 
          length is 14 characters.

     -norestart
          Passes the '-norestart' option to xtserv if xtris can't
          find  a server running and starts one of its own.  This
          has the effect of telling the server not  to  automati-
          cally restart a game after everyone save one player has
          lost.

     -nospeedup
          Passes the '-nospeedup' option to xtserv if xtris can't
          find  a server running and starts one of its own.  This
          has the effect of telling the server not  to  automati-
          cally increase the game speed.

     -s   Starts the game in  standalone  mode.   In  this  mode,
          xtris will not connect to a server, will not be able to
          interact with other players, and will not  be  able  to
          start bots.

     server.name
          Specifies the machine on which an xtris server (xtserv)
          runs.   xtris  will  connect  to  that  server, and add
          itself to the game.

          If the server is not specified, xtris will  attempt  to
          connect  to  a  server on localhost (127.0.0.1).  If no
          server is running on localhost, xtris  will  start  one
          (with  the '-once' option so it will not stay after the
          client exits).

     port 
	  Specifies the port to connect to,  if  the  server  was
          started on a port other than the default (19503).


xtris also recognizes a number of X resources, all in the form

	xtris.something:  value

To redefine keys, the values need to be KeySym names such as 'Return'
or 'q' or 'Backspace'.  For booleans you can use 'true', 'false',
'on', or 'off'.

The resources to set keys are:

leftBorderKey, leftKey, rotateKey, rightKey, rightBorderKey, downKey,
dropKey, startKey, quitKey, pauseKey, botKey, modeKey, msgsKey,
toggleNextKey, clearKey, verboseKey, warpKey, zeroKey.

xtris also understands these resources:

     name           Specifies the player's nickname.  The default
                    is the login name.

     big            Specifies whether  xtris  should  use  larger
                    graphics or not.  The default is ``false''.

     flashy         Specifies whether xtris should  use  flashier
                    colors or not.  The default is ``false''.

     bw             Specifies whether xtris should  start  up  in
                    black   &   white   mode.    The  default  is
                    ``false''.

     verbose        Specifies whether xtris should  start  up  in
                    verbose mode.  The default is ``false''.

     scrollLines    Specifies the size (in lines) of the  message
                    area.   The  default  is  10, and the allowed
                    values are from 3 to 30.

     background     Specifies the color used for the background.


## xtserv

If you you start xtserv manually, you do it like this:

  xtserv [ -help ] [ -once ] [ -daemon ] [ -v ] [ -norestart ] 
	 [ -nospeedup ] [ -p port ]

and the options mean:

     -daemon
          Makes xtserv detach from its controlling terminal, fork
          twice,  close  all  its file descriptors and change the
          current directory to '/'.  Useful to start  xtserv  via
          rsh or from scripts.

     -help
          Prints out a summary of the command-line options xtserv
          recognizes.

     -once
          With this option, xtserv will exit  after  all  clients
          have disconnected.

     -v   Sets verbose mode: xtserv  will  print  information  on
          clients   connecting,  disconnecting,  starting  games,
          sending each other lines, losing, etc.

     -norestart
          With this option, xtserv will not start a new game when
          all  players  but one have lost.  The players will have
          to press the 'start' button themselves to start.

     -nospeedup
          With  this  option,  xtserv  will   not   automatically
          increase  the  game speed every time some user's number
          of lines is greater than 10 x the current level.

     -p   Specifies a port for xtserv to  use.   The  default  is
          19503.


xtserv will automatically restart a game when the one-but-last 
player has lost, unless started with the option -norestart.  It will
not automatically restart a game if the user is playing alone.

xtserv will also drop all bots when there are only bots left in the
game.

The server can handle clients exiting, being killed or losing the
connection even in the middle of a game.

Killing an xtserv will cause all connected clients to exit.


## xtbot

If you you start xtbot manually, you do it like this:

  xtbot [ -help ] [ -n nick ] [ -quiet ] [ -test ] [ server.name [ port ] ]

and the options mean:

     -help
          Prints out a summary of the command-line options  xtbot
          recognizes.

     -n   Sets the nickname for the bot;  the nickname appears at
          the bottom of the screen on xtris clients.

     -quiet
          Sets quiet mode: xtbot will not output any  diagnostics
          or error messages.  xtris always starts xtbot with this
          option.

     -test
          Runs the bot in test mode: xtbot will not connect to  a
          server  or  show  the  game  in progress, but will just
          simulate 16 games as fast as it can and print stats  on
          the  number of lines it did and the number of bricks it
          played.

     server.name
          Specifies the machine on which an xtris server (xtserv)
          runs.   xtbot  will  connect  to  that  server, and add
          itself to the game.

          If the server is not specified, xtbot will  attempt  to
          connect  to  a  server on localhost (127.0.0.1).  If no
          server is running on localhost, xtbot will exit with an
          error message.

     port 
	  Specifies the port to connect to,  if  the  server  was
          started on a port other than the default (19503).


xtbot is the robot (i.e automatic player) for xtris.  xtbot connects to
an xtris server, registers itself as a bot, and simulates a game of
Tetris whenever a human player hits 'start'.

The current version of xtbot uses a pretty good decision algorithm,
which usually does several thousand lines before losing, when playing on
its own.  When playing against a bot, though, the main limiting factor
is the speed, which is why xtbot purposefully waits a little before
dropping each brick, so that humans can compete speed-wise.

xtbot is started automatically by xtris, with the option '-quiet', when
a player presses the 'bot' button.

Running copies of xtbot can be killed either by clicking on their name
from an xtris window, or by killing the process. 

You can make your own bots based on your favorite decision algorithms.
For this the easiest way is to change the decision function in decide.c
while keeping the rest of the bot's skeleton (in xtbot.c).  See the
comments in decide.c, decide.h and xtbot.h for details about the
interface between these.  Alternatively, the protocol between the client
and the server is described in detail in the file PROTOCOL, so you can
make completely independent bots.

The default algorithm for the bot depends on 6 coefficients to evaluate
each possible position of the piece.  You can set the environment
variables XTBOT_FRONTIER, XTBOT_HEIGHT, XTBOT_HOLE, XTBOT_DROP,
XTBOT_PIT, XTBOT_EHOLE.  See the file decide.c to see what they do.

The values for the coefficients that xtbot uses now were obtained with a
genetic algorithm using a population of 50 sets of coefficients,
calculating 18 generations in about 500 machine-hours distributed among
20-odd Sparc workstations.  This improved the average number of lines
from 10,000 to about 50,000.  The code used for this isn't nearly clean
enough to distribute in a release.  If you're interesed, please e-mail
the author privately.


## bugs, things to be improved

There is no way to tell xtris to use keys that do not map to ASCII
codes, or to redefine the player's keys from the X interface.

The colors are rather ugly.

The bots' algorithm could still be improved to try to make serveral
lines at a time.

If you find bugs, please report them to me.


## acknowledgements

Some bits of X11 code are based on xtet42, a 2-player tetris by Hugo
Eide Gunnarsen of the Norwegian Institute of Technology.

Specifically, init_X(), SetColor(), DrawSqr() and DrawButton() are more
or less modified versions of the same functions in xtet42.

Also thanks to Fabrice Noilhan <Fabrice.Noilhan@ens.fr> and Denis Auroux
<Denis.Auroux@ens.fr> for debugging help, and Laurent Bercot
<Laurent.Bercot@ens.fr> and Sebastien Blondeel
<Sebastien.Blondeel@ens.fr> for ideas for the bot's decision algorithm.


## license

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.



For bug reports, comments, questions, email roger.espel.llima@pobox.com

(For win32 version please email vvidovic@inet.hr)

