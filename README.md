# Pyramid

Pyramid, a Patience Card Game for the Psion Series 3

&copy;1993 J Cade Roux

Released under the GNU General Public License v2.

## Installation

To install Pyramid, copy `PYRAMID.APP` it to any directory (like `\APP\`) and
install it <Psion-I> from the system screen.  Alternatively, rename it
`PYRAMID.IMG`, and copy it to the `\IMG\` directory, or any other directory
(such as a games alias) referenced by an alias of RunImg.

## Rules

This is the Pyramid patience card game. The tableau is in the form of
a pyramid with seven rows. The object of the game is to remove as many
cards from the pyramid as possible.

Only those cards which are not overlapped by other cards can be removed.
A pair of cards which total 13 in value, or a singleton King may be removed.

The face down pile is the hand which is turned up one card at a time onto
the talon pile, of which the top card may also be used to make a match.

The talon pile is not recycled. The Kings and pairs are discarded.

## Playing

The keys used to control Pyramid are as follows:

**Left** and **Right** arrow keys move the pointer to the card to be lifted
(the arrow point diagonally up from the left of the card). The **space
bar** is pressed, and the card is picked up (unless it is a King, in
which case it is immediately discarded).  Then the card is moved to the
destination and the space bar is pressed to attempt to match the card.

If a match is made the two cards will be discarded.

**Return** draws a single card from the face down hand if the player is
currently not holding a card. **Escape** aborts a move before dropping.

The rules are simply as described above. Pressing the **space bar** when
on the face down pile flips a card onto the talon pile. The talon pile
may not be recycled.

## Compiling

Development tools used included:

On the PC:

- The **Psion SIBO SDK** with **TopSpeed C** compiler

On the Amiga:

- **DeluxePaint IV** - the only paint program I've ever used which is worth
running.  Certainly light years ahead of Paintbrush - the only other
development tool I have used to make graphics for Psion games.

Pyramid was written completely with PLIB, HWIF, and WLIB functions,
and, as such, is not portable (even to older SIBO machines, unless you
want to rewrite all the HWIF sections) in any way.

It will run fine on later SIBO/EPOC16 machines, such as the Series 3a/c/mx and Siena.

## Problems/Bugs

Currently no undo other than cancelling the current move.

## Revisions

1.0a: Initial release
