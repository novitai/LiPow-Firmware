- Download the [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) and [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html). Both of these softwares can be downloaded from `st.com` with the provided links. You need to create an account to be able to download the programs.
- The IDE is used for writing/editing/debugging the program. You will use it to compile the code. It is a loaded IDE with way too many options.
- The Programmer is used for embedding the compiled binary into your STM32 board. You will need a ST-Link/v2 to be able to make the proper connection between your computer and the STM32 board.
## STM32CubeIDE
- Once opened you will be prompted with a workspace selection. Select a directory and use it as workspace. Each workspace can have its own workspace.
- Once the workspace is selected, open the preferred project folder.
- Debug the script. You might need to edit project build settings depending on your device.
	1. Open project properties:
		- Right-click on your project in the Project Explorer.
		- Select `Properties`
	2. Navigate to C/C++ Build Settings:
		- In the Properties window, expand `C/C++ Build`
		- Select `Settings`
	3. Modify the Post-build Command:
		- In the `Settings` window, go to the `Build Steps` tab.
		- In the `Post-build steps` section, you should see the command for generating the hex file.
		- Change the command from `arm-none-eabi-objcopy.exe -) ihex "Lipow.elf" "Lipow.hex"` to `arm-none-eabi-objcopy -O ihex "Lipow.elf" "Lipow.hex"` if you are not on a Windows machine since `.exe` is for Windows OS.
	4. Apply and Build:
		- Click `Apply` and `OK`
		- Build the project
- The debug process should produce the necessary binary, in this case `Lipow.elf`. Once that file is generated move to the STM32CubeProgrammer to embed the binary into your board.
## STM32CubeProgrammer
- Have the ST-Link/v2 ready, connect the USB end to your computer (we haven't managed to properly use it in a MAC, the best option is to use a Windows for now).
- Press refresh button next to the `Serial Number` dropdown. The ST-Link/v2 you are using should be visible.
- Connect VCC, GND, SWDIO, SWCLK, RST on ST-Link/v2 to STM32 board.
- Press `Connect` to connect to the board.
- Press `Open file` and open the binary file that you have compiled.
- Write the binary to the board.
- Go back to `Device memory` and read, you should be able to see the written binary.