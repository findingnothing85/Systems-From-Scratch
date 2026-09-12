## How to know your ip address?

In terminal, first try

```
ipconfig getifaddr en0
```

Then try

```
ipconfig getifaddr en1
```

You should see it. 

## How to compile and run your C programs?

Just

```
gcc fileserver.c -o fileserver
```

then

```
./fileserver
```

and `fileserver` is running. Similarly for `fileclient`.
