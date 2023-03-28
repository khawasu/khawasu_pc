### Example usage:
```bash 
hwctrl --net "dev network" --psk "123" -a 1 --socket client 127.0.0.1 49152 \
execute 1 1 power_state 1
```
Connect to fresh **network = "dev network"** 
with **self-assigment addr = "1"** and **psk = "123"**
with **socket interface** with **hostname 127.0.0.1** and **port 49152**
and **execute action "power_state"** 
with **arg = 1** on device 
with **far_addr = 1** and **log_addr = 1** 

```bash 
hwctrl --net "dev network" --psk "123" -a 1 --socket client 127.0.0.1 49152 \
devices
```
Connect to fresh **network = "dev network"**
with **self-assigment addr = "1"** and **psk = "123"**
with **socket interface** with **hostname 127.0.0.1** and **port 49152**
and **show devices list**


## Shell arguments documentation
Load specified config file: 
`-c <config name>` or `--config <config name>` 

### Fresh setting
Set network name:
`-n <name>` or `--net <name>`

Set network PSK:
`--psk <psk>` 

Set self-assigned network address:
`-a <addr>` or `--addr <addr>`  

### Fresh register interfaces
Register socket client interface:
`--socket client <addr> <port>`

Register socket server interface:
`--socket server <addr> <port>`

Register COM interface (P2PUnsecuredShortInterface):
`--com <path> <baudrate>`

### Khawasu setting
Enable debug info
`--debug`

## Config documentation
You can create a config file to show how hwctrl should work.
For usage:
```bash
hwctrl -c <config filename>
hwctrl --config <config filename>
```

Below is the config file with descriptions of each option in the **toml** language:

```toml
# A section about fresh configuration
[fresh]
net = "dev network" # Network name <string>
psk = 1234 # Network pre-shared Key <string>
addr = 2 # Self-assigned network address <integer>

# A section about registered fresh interfaces
# sections must be defined by this pattern:
# [fresh.interfaces.<interface_type>.<name>]
# where <name> can be any value, but <interface_type> in:
# socket
# com
#
# You can multiple define interface with some types, but 
# <name> should be different
[fresh.interfaces]
    # fresh.interfaces.socket describes FreshSocketInterface
    # by passing type = "client" you create socket.client
    # which connecting to another socket.server with host:port
    # where host <string> and port <integer>
    [fresh.interfaces.socket.client]
    type = "client"
    host = "10.0.0.1"
    port = 49152

    # by passing type = "server" you create socket.server
    # and another socket.client can be connected to him
    # where host <string> and port <integer>
    [fresh.interfaces.socket.server]
    type = "server"
    host = "0.0.0.0"
    port = 49152
    
    # fresh.interfaces.com describes P2PUnsecuredShortInterface COM Port
    # you must define path (path <string>) 
    # and boudrate (baud <integer>) this serial port
    [fresh.interfaces.com.com1]
    path="\\.\COM1"
    boud=115200
    
# A section about khawasu configuration
[khawasu]
# Enables interactive shell for working with devices from console
enable_interactive_shell = true 
    
    # A section about khawasu device manager
    # Must be configaurated for interactive shell
    [khawasu.device_manager]
    # If "auto" then hwctrl will generate randomness port
    logical_port = 9999
```