# `mysh`: A Simple Shell That Does Nothing

# Build
Use `make all` to build all necessary stuff.

# Feature
We currently support all basic feature, including input 
output redirection and pipeline.

You can leave the shell by sending EOF(Ctrl+D).

The following commands have been tested.
- `ls -al`
- `sleep 10`
- `ls -la > /tmp/x`
- `cat < /etc/passwd`
- `less /etc/passwd`
- `cat /etc/passwd | cat | less`
- `ps -o pid,sid,pgid,ppid,cmd | cat | cat | tr A-Z a-z`

**Note**: Sometimes commands may not work correctly, you
may need to restart the shell to avoid this bug. T^T

We support redirect stdin and stdout simultaneously, but 
we need to redirect stdin before redirect stdout.
```
<command> > <outfile> < <infile>
cat > ./flag < ./data
```