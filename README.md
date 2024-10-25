<h1 align="center">
  Web Filter
</h1>
<p align="center">
  <img src="https://github.com/user-attachments/assets/4ac08b9b-aff3-42cb-bb59-b30bdf11d709"></img>
</p>

Command-line based web proxy that filters and blocks access to webpages containing specified keywords. Designed for Unix-like systems.
<br/>

Compile with `gcc proxy.c -o proxy` and run with `./proxy`. See the file for necessary information about ports.
<br/>

While the script is running, use `BLOCK <keyword>` to add a keyword to the block list, and `UNBLOCK` to unblock all keywords. 
