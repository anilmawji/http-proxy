<h1 align="center">
  Web Filter
</h1>

Command-line based web proxy that filters and blocks access to webpages containing specified keywords. Designed for Unix-like systems.
<br/>

Compile with `gcc proxy.c -o proxy` and run with `./proxy`. See the file for necessary information about ports.
<br/>

While the script is running, use `BLOCK <keyword>` to add a keyword to the block list, and `UNBLOCK` to unblock all keywords. 
