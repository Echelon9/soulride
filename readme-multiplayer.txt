
The basic idea and programming style is the following:

Not all components need to be aware of the fact that there are several players. Quite often, there's a function call that hides this fact, such as Game::GetCurrentPlayer()

Functions that want to know which is the current player can use the MultiPlayer module:
int player_index = MultiPlayer::CurrentPlayerIndex()
Player 1-4 are indexed 0-3 in the arrays.

The number of player can only be changed in the main menu, check uimainmenu.cpp for details.

The rendering is done by "painting" each player one time, see gameloop.cpp for details.

