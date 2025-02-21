# jbot_mm

HEY ALL THIS IS NOW PUBLIC FOR EVERYONE SINCE TF2 GOT A PROPER SDK RELEASE

THIS IS PUBLIC NOW SO THAT YALL CAN SEE HOW TERRIBLE THINGS USED TO BE

i would not recommend yall actually build this since this is for a real old version of tf2 and i obviously have not updated the datatables

also this doesnt even WORK (properly, how I want it to). LOL!!!!

old readme
------

THIS IS UNFINISHED. and i probably wont finish this

this is a mm:s plugin for tf2 intended to create Realistic and Believable player bots. intended to replace tf2's stock tfbot system


mm:s readme is beloww

For more information on compiling and reading the plugin's source code, see:

	http://wiki.alliedmods.net/Category:Metamod:Source_Development

Build instructions
------------------

Make sure ambuild2 is installed: https://github.com/alliedmodders/ambuild

Configure the build (`--hl2sdk-root` specifies the path where the all SDKs have been installed by `support/checkout-deps.sh` and `--mms_path` is the path to Metamod: Source).

If you only want to compile using a specific SDK you can hack around the requirement to build for all SDKs by modifying the calls to the SDK constructor in the assignment to `PossibleSDKs` in `AMBuildScript` and setting the `platforms` parameter to \[\] for all SDKs that you don't want to compile for.

### Windows

Use Command Prompt as Gitbash doesn't handle the path arguments correctly.

```
mkdir build
cd build
python ../configure.py --hl2sdk-root C:\Users\user\Documents\GitHub --mms_path C:\Users\user\Documents\GitHub\metamod-source
```

Build:
```
ambuild
```
