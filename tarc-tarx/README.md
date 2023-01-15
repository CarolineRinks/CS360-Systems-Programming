# tarc and tarx

CS360: Systems Programming

-----------
Description
-----------
The program tarc creates a tar file and works similarly to the command "tar cf".

The program tarx recreates a directory from a tarfile, preserving protections and 
modification times for each file. It works similarly to the command "tar xf".

src/ contains the source code for tarc and tarx.

ex/ contains some files that can be used to test tarc and tarx.

libfdr/ contains header files for double-linked lists, red-black trees, and input processing.

-----
Usage
-----

      ./tarc directory > directory.tarc
      
      ./tarx < directory.tarc

