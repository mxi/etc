#!/usr/bin/python
#
# The red-black tree is something that we cover in school but rarely does the
# instructor allow the student to get into perilous danger by actually
# implementing the damn thing.
#
# So here we are. Implementing a red-black tree. With the help of wikipedia.
# Thanks wikipedia.
#
# This program is a red-black tree visualizer using curses. You can add and
# delete elements sequenctially using the familiar vim/postscript operators:
#
#    Ni      - inserts an element with value N
#    Nd      - deletes element with value N
#    R       - generate a random RB tree
#    D       - try to delete some random elements from the current tree
#    h,j,k,l - move the tree view left, down, up, and right.
#    w       - display a dummy warning message.
#    q       - exit the program.
#
# Author: Maxim Kasyanenko (for legal purposes)
# License: MIT
#
import sys
import curses
import random

class NodeRB:
    """
    self._children[0] := left child,
    self._children[1] := right child,
    """

    BLACK = 0
    RED = 1

    @staticmethod
    def insert_into(root, value):
        if root is None:
            new = NodeRB(value)
            new._percolate()
            return new
        root.insert(value)
        while root.parent:
            root = root.parent
        return root

    @staticmethod
    def delete_from(root, value):
        if root is None:
            return None
        survivor = root.delete(value)
        if survivor:
            while survivor.parent:
                survivor = survivor.parent
        return survivor

    def __init__(self, value, color=None, parent=None, left=None, right=None):
        self.value = value
        self.color = color
        self.parent = parent
        self.children = [ left, right ]

        if self.color is None:
            self.color = NodeRB.RED

    @property
    def label(self):
        return str(self.value)

    def insert(self, value):
        side = 0 if value <= self.value else 1
        child = self.children[side]
        if child:
            child.insert(value)
        else:
            infant = NodeRB(value, parent=self)
            self.children[side] = infant
            infant._percolate()

    def delete(self, value):
        if value == self.value:
            return self._delete()
        elif value < self.value and self.children[0]:
            self.children[0].delete(value)
        elif self.value < value and self.children[1]:
            self.children[1].delete(value)
        return self

    def _delete(self):
        n_children = 0
        n_children += 1 if self.children[0] else 0
        n_children += 1 if self.children[1] else 0
        survivor = self

        if self.color == NodeRB.RED:
            assert n_children != 1
            if n_children == 0:
                assert self.parent
                survivor = self.parent
                survivor.children[survivor._sideof(self)] = None
                self.parent = None
            else:
                usurper = self._findusurper()
                self.value = usurper.value
                usurper._delete()
        else:
            assert self.color == NodeRB.BLACK
            if 0 < n_children:
                usurper = self._findusurper()
                self.value = usurper.value
                usurper._delete()
            if n_children == 0:
                # This is where the magic happens ...
                survivor = self._delete_as_black_leaf()

        return survivor

    def _delete_as_black_leaf(self):
        assert self.color == NodeRB.BLACK
        assert self.children[0] is None
        assert self.children[1] is None
        survivor = self.parent

        if self.parent is None:
            return None # final node removed, no survivors

        N = self
        P = self.parent
        side = P._sideof(N)
        P.children[side] = None # remove N on first iteration because that's the target

        # analogy to 2-4 tree: we have a setup like this
        #
        #           [P]
        #           | |
        #         [N] [S]
        #         /|   |\
        #            [C][D]
        #
        # clearly, we can merge the black key S into the same node as P, making
        # it red, and therefore decreasing the level.
        #
        #         [- P S]
        #           /  |\
        #         [N][C][D]
        #
        # Since we decreased the entire level for the subtree of P, we iterate
        # up until we rebalance the thing.
        #

        # delete cases:
        # 1 - S is red, everything else is black
        # 2 - P is red, everything else is black
        # 3 - C is red, S and D are black
        # 4 - D is red, S is black
        delete_case = 0 

        while P:
            S = P.children[1-side]
            assert S # otherwise, we violate the invariant black node count per path requirement
            D = S.children[1-side]
            C = S.children[  side]
            if S.color == NodeRB.RED:
                delete_case = 1
                break
            # If C or D is red, then the 2-4 equivalent C-S-D node can donate a key to extend
            # the N side of P by one black node.
            if D is not None and D.color == NodeRB.RED:
                delete_case = 4
                break
            if C is not None and C.color == NodeRB.RED:
                delete_case = 3
                break
            if P.color == NodeRB.RED:
                delete_case = 2
                break
            S.color = NodeRB.RED
            N = P
            P = N.parent
            side = P._sideof(N) if P else None

        # we're done
        if P is None:
            return survivor

        if delete_case == 1:
            P._rotate(side)
            assert P.parent == S
            P.color = NodeRB.RED
            S.color = NodeRB.BLACK
            S = P.children[1-side] # new S is the previous C
            assert S # otherwise the previous S wasn't red so something is wrong
            D = S.children[1-side]
            C = S.children[  side]
            if D is not None and D.color == NodeRB.RED:
                delete_case = 4
            elif C is not None and C.color == NodeRB.RED:
                delete_case = 3
            else:
                delete_case = 2

        if delete_case == 2:
            P.color = NodeRB.BLACK
            S.color = NodeRB.RED
            delete_case = -1

        if delete_case == 3:
            S._rotate(1-side)
            assert C.parent == P
            C.color = NodeRB.BLACK
            S.color = NodeRB.RED
            D = S
            S = C
            assert S and S.color == NodeRB.BLACK
            assert D and D.color == NodeRB.RED
            delete_case = 4

        if delete_case == 4:
            P._rotate(side)
            assert P.parent == S
            S.color = P.color
            P.color = NodeRB.BLACK
            D.color = NodeRB.BLACK

        return survivor


    def _findusurper(self):
        drill_direction = 0 if self.children[1] else 1
        drill_node = self.children[1-drill_direction]
        assert drill_node
        while drill_node.children[drill_direction]:
            drill_node = drill_node.children[drill_direction]
        return drill_node

    def _percolate(self):
        N = self

        # we're a true root node, so we don't care
        if N.parent is None:
            N.color = NodeRB.BLACK
            return

        # if our parent is the boss, all good
        P = N.parent
        if P.color == NodeRB.BLACK:
            return

        # if the parent is not the boss, contact upper management
        G = P.parent
        U = G._siblingof(P)
        Ucolor = U.color if U else NodeRB.BLACK

        if Ucolor == NodeRB.RED:
            G.color = NodeRB.RED
            P.color = NodeRB.BLACK
            if U:
                U.color = NodeRB.BLACK
            G._percolate()
            # analogy to 2-4 tree: the full 4-node is broken apart (G becomes
            # a "free-floating", hence red node, and the recoloring process that 
            # proceeds recursively is the pushing of G into the above 2-4 node.)
        else:
            Pside = G._sideof(P)
            Nside = P._sideof(N)
            if Pside != Nside:
                P._rotate(1-Nside)
                P = N
            G._rotate(1-Pside)
            P.color = NodeRB.BLACK
            G.color = NodeRB.RED
            # analogy to 2-4 tree: think of N as being an "excess" red node on the side
            # of P not in a separate 2-4 container. Thus, N is "outside" of the box:
            #
            #      N -> [P G _]
            #           | | |
            #         [0][0][X U Y]
            #
            # The rotate between P/N (if there is one) orders P/N to fit in the 4-node,
            # and the next rotation and recoloring of G is simply the pushing of N into
            # the 4-node, making P the boss black node:
            #
            #
            #           [N P G]
            #           | | | \
            #         [0][0][0][X U Y]
            #

    def _rotate(self, toward):
        assert 0 <= toward <= 1
        parent = self.parent
        usurper = self.children[1-toward]
        assert usurper
        self.children[1-toward] = None
        self._adopt(usurper.children[toward], 1-toward)
        usurper.children[toward] = None
        usurper._adopt(self, toward)
        usurper.parent = parent
        if parent:
            side = parent._sideof(self)
            parent.children[side] = usurper

    def _siblingof(self, child):
        if self.children[0] is child:
            return self.children[1]
        if self.children[1] is child:
            return self.children[0]
        assert False # internal function, param must be correct

    def _sideof(self, child):
        if self.children[0] is child:
            return 0
        if self.children[1] is child:
            return 1
        assert False # internal function, param must be correct 

    def _adopt(self, child, side):
        assert 0 <= side <= 1
        assert self.children[side] is None
        self.children[side] = child
        if child:
            child.parent = self


class VisualRB:
    """
    Describes a visual bounding box for a subtree.
    """

    @classmethod
    def compute(cls, node: NodeRB):
        if node is None:
            return None

        r = cls(node)
        r.west = VisualRB.compute(node.children[0])
        r.east = VisualRB.compute(node.children[1])
        r.label = node.label

        r.right = len(r.label)
        r.bottom += 1
        r.labelpos = 0
        r.labelmid = len(r.label)//2
        west_bottom = 0
        east_bottom = 0

        # The magic "2" y-offset below is to make room for (1) the root label
        # and (2) the box drawing characters to connect the root, west, and east.
        if r.west:
            r.west.parent = r
            r.west.move(0, 2)
            r.right += r.west.width
            r.labelpos += r.west.width
            r.labelmid += r.west.width
            west_bottom = 2 + r.west.height
        if r.east:
            r.east.parent = r
            r.east.move(r.right, 2)
            r.right += r.east.width
            east_bottom = 2 + r.east.height

        r.bottom = max(west_bottom, east_bottom)

        return r

    def __init__(self, node):
        """
        All coordinates are in screen space (that is, origin in the top left, x-axis
        extends to the right, y-axis extends down.)

        All coordinates are with respect to the origin. self.offx, self.offy are offset
        with respect the parent. self.abx, self.absy are offsets with respect to the
        screen computed recursively using parent absx, absy and local offx, offy.

        self.top := y-coordinate of the top of the visual.
        self.right := x-coordinate of the right of the visual.
        self.bottom := y-coordinate of the bottom of the visual.
        self.left :=  x-coordinate of the left of the visual.
        self.labelpos := x-coordinate of the left-hand side of the label
        self.labelmid := x-coordinate of the mid-position for the label
        self.parent := parent VisualRB
        self.west := left child VisualRB
        self.east := right child VisualRB
        self.node := NodeRB
        self.offx := x-offset with respect to the parent VisualRB
        self.offy := y-offset with respect to the parent VisualRB
        self.absx := x-offset with respect to the screen
        self.absy := y-offset with respect to the screen
        """
        self.top = 0
        self.right = 0
        self.bottom = 0
        self.left = 0
        self.labelpos = 0
        self.labelmid = 0
        self.parent = None
        self.west = None
        self.east = None
        self.node = node
        self.offx = 0
        self.offy = 0
        self.absx = 0
        self.abxy = 0

    def move(self, dx, dy):
        self.offx += dx
        self.offy += dy

    @property
    def width(self):
        return self.right - self.left

    @property
    def height(self):
        return self.bottom - self.top


class CursesWindowWrapper:

    def __init__(self, window):
        self._window = window
        self._maxy = 0
        self._maxx = 0
        self._offy = 0
        self._offx = 0
        self.resizenotify()

    def useoffset(self, y, x):
        self._offy = y
        self._offx = x

    def putrow(self, y, x, content, attrs=0, times=1):
        ay = self._offy + y
        ax = self._offx + x

        if ay < 0 or self._maxy <= ay:
            return

        length = len(content) if type(content) is str else 1
        x0 = max(0, ax)
        x1 = min(self._maxx, ax + length * times)

        for i in range(x0, x1):
            ch = None
            if type(content) is str:
                ch = ord(content[(i-ax) % length])
            else:
                ch = content
            self.addch(ay, i, ch, attrs)

    def resizenotify(self):
        y, x = self._window.getmaxyx()
        self._maxy = y
        self._maxx = x

    def __getattr__(self, name):
        return getattr(self._window, name)


class Program:
    COLOR_DEFAULT    = 0
    COLOR_ERROR      = 1
    COLOR_NODE_RED   = 2
    COLOR_NODE_BLACK = 3

    def __init__(self, stdscr):
        curses.set_escdelay(8)
        stdscr.clear()

        self.stdscr = CursesWindowWrapper(stdscr)
        self.cmdwin = CursesWindowWrapper(stdscr.subwin(1, curses.COLS, 0, 0))
        self.viswin = CursesWindowWrapper(stdscr.subwin(1, 0))

        curses.init_pair(Program.COLOR_ERROR, curses.COLOR_BLACK, curses.COLOR_YELLOW)
        curses.init_pair(Program.COLOR_NODE_RED, curses.COLOR_BLACK, curses.COLOR_RED)
        curses.init_pair(Program.COLOR_NODE_BLACK, curses.COLOR_BLACK, curses.COLOR_WHITE)

        self.running = True
        self.vis_x = 0 
        self.vis_y = 0
        self.root = None
        self.number = ""
        self.error = None
        self.ignore_resize = False

    def loop(self):
        curses.ungetch(0)
        while self.running:

            # input
            ch = self.stdscr.getch()
            if ch == ord('q'):
                self.running = False
            elif ch == ord('h'): # move view left
                self.vis_x += 2
            elif ch == ord('j'): # move view down
                self.vis_y -= 1
            elif ch == ord('k'): # move view up
                self.vis_y += 1
            elif ch == ord('l'): # move view right
                self.vis_x -= 2
            elif ch == ord('r'): # origin
                self.vis_x = 0
                self.vis_y = 0
            elif ch == ord('R'): # random tree
                self.root = None
                n = random.randint(1,100)
                for i in range(n):
                    self.root = NodeRB.insert_into(self.root, random.randint(1, 100))
            elif ch == ord('D'):
                n = random.randint(1,100)
                for i in range(n):
                    self.root = NodeRB.delete_from(self.root, random.randint(1, 100))
            elif ch == ord('w'): # stupid error
                self.error = "This is a built-in error to test the messaging thing."
                self.number = ""
            elif ch == ord('i'): # insert node
                self.error = None
                if len(self.number) == 0:
                    self.error = "Number expected before insert (i) operator."
                else:
                    self.root = NodeRB.insert_into(self.root, int(self.number))
                    self.number = ""
            elif ch == ord('d'): # delete node
                self.error = None
                if len(self.number) == 0:
                    self.error = "Number expected before delete(d) operator."
                else:
                    self.root = NodeRB.delete_from(self.root, int(self.number))
                    self.number = ""
                    # self.error = "Delete (d) operator not implemented yet."
            elif ch == 27: # escape
                self.error = None
                self.number = ""
            elif ord('0') <= ch <= ord('9'):
                self.error = None
                if len(self.number) < 8:
                    self.number += chr(ch)
            elif ch == curses.KEY_DC \
              or ch == curses.KEY_BACKSPACE: # delete operand digit
                self.number = self.number[:-1]
            elif ch == curses.KEY_RESIZE:
                if not self.ignore_resize:
                    y, x = self.stdscr.getmaxyx()
                    curses.resizeterm(y, x)
                    self.cmdwin.resizenotify()
                    self.viswin.resizenotify()
                    self.ignore_resize = True
                else:
                    self.ignore_resize = False

            # draw tree
            self.viswin.move(0, 0)
            self.viswin.clear()

            visual = VisualRB.compute(self.root)
            bfs = []

            if visual:
                y, x = self.viswin.getmaxyx()
                visual.offx = (x - visual.width)//2
                visual.offy = (y - visual.height)//2 
                bfs.append(visual)

            while bfs:
                n = bfs.pop()

                if n.parent:
                    n.absx = n.parent.absx + n.offx
                    n.absy = n.parent.absy + n.offy
                else:
                    n.absx = n.offx
                    n.absy = n.offy
                
                attrs = 0
                if n.node.color == NodeRB.RED:
                    attrs |= curses.color_pair(Program.COLOR_NODE_RED)
                elif n.node.color == NodeRB.BLACK:
                    attrs |= curses.color_pair(Program.COLOR_NODE_BLACK)
                else:
                    assert False

                self.viswin.useoffset(self.vis_y + n.absy, self.vis_x + n.absx)
                # TODO self.viswin.isrectvisible
                self.viswin.putrow(0, n.labelpos, n.node.label, attrs)
                if n.west:
                    bfs.append(n.west)
                    edge = n.labelmid - (n.west.offx + n.west.labelmid) - 1
                    self.viswin.putrow(1, n.west.labelmid, curses.ACS_ULCORNER)
                    self.viswin.putrow(1, n.west.labelmid+1, curses.ACS_HLINE, times=edge)
                    self.viswin.putrow(1, n.labelmid, curses.ACS_LRCORNER)
                if n.east:
                    bfs.append(n.east)
                    edge = (n.east.offx + n.east.labelmid) - n.labelmid - 1
                    self.viswin.putrow(1, n.labelmid, curses.ACS_LLCORNER)
                    self.viswin.putrow(1, n.labelmid+1, curses.ACS_HLINE, times=edge)
                    self.viswin.putrow(1, (n.east.offx + n.east.labelmid), curses.ACS_URCORNER)
                if n.west and n.east:
                    self.viswin.putrow(1, n.labelmid, curses.ACS_BTEE)

            self.viswin.noutrefresh()
            
            # draw command area
            self.cmdwin.move(0, 0)
            self.cmdwin.clrtoeol()
            if self.error:
                curses.curs_set(False)
                self.cmdwin.addnstr(self.error, curses.COLS)
                self.cmdwin.chgat(0, 0, curses.COLS, curses.color_pair(Program.COLOR_ERROR))
            else:
                attrs = curses.color_pair(Program.COLOR_DEFAULT) | curses.A_UNDERLINE
                curses.curs_set(True)
                self.cmdwin.chgat(0, 0, curses.COLS, attrs)
                self.cmdwin.addnstr(self.number, curses.COLS, attrs)

            self.cmdwin.noutrefresh()

            # finish
            curses.doupdate()

        self.stdscr.refresh()

    @classmethod
    def main(cls, stdscr):
        program = cls(stdscr)
        if 0:
            program.root = \
            NodeRB(2, color=NodeRB.BLACK,
                left=NodeRB(1, color=NodeRB.BLACK),
                right=NodeRB(4, color=NodeRB.RED,
                    left=NodeRB(3, color=NodeRB.BLACK),
                    right=NodeRB(6, color=NodeRB.BLACK,
                        left=NodeRB(5, color=NodeRB.RED),
                        right=NodeRB(7, color=NodeRB.RED)
                    )
                )
            )
        if 0:
            program.root = \
            NodeRB(2, color=NodeRB.BLACK,
                left=NodeRB(1),
                right=NodeRB(3)
            )
        if 0:
            program.root = \
            NodeRB(2, color=NodeRB.BLACK,
                left=NodeRB(1),
            )
        if 0:
            for i in range(100):
                program.root = NodeRB.insert_into(program.root, random.randint(1, 100))
        if 0:
            program.root._rotate(0)
            while program.root.parent:
                program.root = program.root.parent
        program.loop()


if __name__ == "__main__":
    curses.wrapper(Program.main)
