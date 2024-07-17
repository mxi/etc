A collection of experimental programs.

<h2> C </h2>

<details> 
<summary> 
    <b><a href="c/load_terminfo.c">load_terminfo.c</a></b> - Dump a
    subset of parsed terminfo without using <code>infocmp</code>.
</summary>

<img src="/readme/load_terminfo.png" with="35%"/>

</details>

<h2> Python </h2>

<details> 
<summary> 
    <b><a href="py/rb_tree.py">rb_tree.py</a></b> - Curses-based red-black 
    tree visualizer in Python.
</summary>

<img src="/readme/rb_tree.png" width="35%"/>

<p>
    See the the top of the source file for controls.
</p>

<p>
    NOTE: although I've tested the algorithm for insert/delete decently well,
    there is quite a bit flickering when the tree is redrawn. Unfortunately,
    the tree is currently redrawn on every input, so if you hold down any key
    it will flicker to no end.
</p>

<p>
    You can try it in the neovim terminal which doesn't suffer the same
    flickering as a native terminal emulator.
</p>
</details>

<details> 
<summary> 
    <b><a href="py/diamond_mro.py">diamond_mro.py</a></b> - Demonstration of
    Python's breadth first method resolution scheme with inheritance.
</summary>

```
[<class '__main__.D'>, <class '__main__.B'>, <class '__main__.C'>, <class '__main__.A'>, <class 'object'>]
--- d.a() ---
Aa
Da
--- d.b() ---
Ab
Bb
Db
--- d.c() ---
Ac
Cc
Bc
Dc
--- d.d() ---
Bd
Dd
--- d.e() ---
Ce
De
```
</details>



