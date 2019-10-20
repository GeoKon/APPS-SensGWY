### CLI Commands

CLI commands are accessible via the Serial Console, via the WEB interface, or via the Thinger.io Device API. The CLI syntax is trivially simple: <action> followed by one or more arguments <arg1> ... <argn>. Each CLI command translates to a function call within the embedded code.

Embedded applications may also utilize *Named Parameters*. Such parameters can represent either the current state of the application, or non-volatile parameters which are maintained in the FLASH of the device and restored every time the code starts up.

Specifically for the SensGWY application, CLI includes the following:

##### Single letter, often used commands

​    **h**	[mask]. Lists of all commands 
​	**b**	[mask]. Brief help 
​	**p**	shows user parameters (brief) 
​	**w**	shows WiFi parameters (brief) 

##### Diagnostic commands

​	**reset**	Restart/Reboot 
​	**test45**	Flip-flops (LED) (GPIO4) (GPIO5) until CR 
​	**testinp**	Inputs GPIOs: 02, 04, 05, 14 until CR 
​	**inpPIN**	p1 p2 ... pN. Inputs PINS until CR 
​	**outPIN**	pin. Squarewave output to 'pin' until CR 
​	**format**	Formats the filesystem 
​	**dir**	List files in FS 

##### Usual Named Parameter commands

​	**initEE**	Initialize parameters to defaults and save to EEP 
​	**set**	name value. Update parm & save in EEPROM 
​    **get**	name1..nameN. Get parameter values
​	**show**	show all parameters

and the MegunoLink counter parts such as: **!initEE, !set, !show** which perform the identical functions as above, but also update the MegunoLink table of EEPROM parameters

##### Commands relating to the SenseGWY application

​	**status**	Displays wifi status
​    **scan**	0|1|3 Enable scanning sensors. 3=update Meguno 
​	**state**	Shows measurement state
​	**send**	sindex c1..c4. Send /cli?cmd=c1+..+c4 
​	**ask**	si </cmd>|<cmd p1..pn>. Do an HTTP/GET from sensor 
​	**result**	[si]. Shows last result of this sensor 
​	**fetch**	[sensor] Fetch sensor data and decode measurements

and the MegunoLink counterparts, such as: **!status, !fetch, !scan, !state** which update multiple fields of the GUI interface. 

### Examples

**cmd: b**  (brief list of commands)
    `h       b       p       w   reset  test45 testinp  inpPIN  outPIN  formatdir  initEE !initEE     set    !set     get    show   !show !status    sendask  result   fetch  !fetch    scan   !scan  !state` 

**cmd: p**  (list of named parameters & their values)
`trace=0, timeout=2000, dbox=0, mask=XXX--, sens0IP=115, sens1IP=116, sens2IP=117, sens3IP=0, sens4IP=0, `

**cmd: w** (show the WiFi parameters & RSSI)
`SSID:kontopidis2GHz, PWD:123456789a, StIP=, Port:80, RSSI:-65`

### Local and Remote CLI

CLI commands can be invoked in multiple ways:

1. using the Serial Interface (i.e. Console)
2. Using a REST call over HTTP
3. Using the WEBCLI page
4. Using the Thinger.io Device API

The first method is self explanatory: the user types the command and gets the response back.

The second method requires the URL of the device, say 192.168.0.67, followed by **/cli?cmd=CLICMD+CLIAGG1+...CLIARGn**. Example:

- `http://192.168.0.67/cli?cmd=set+timeout+1000`

is identical to the console command:

- `set timeout 1000`

The response of the command is printed as text in the WEB page as shown below.

<img src="file://D:\Dropbox\Arduino\APPLICATIONS\SensGWY\doc-assets\REST CLI.JPG" width="600px" />

The third method utilizes the `webcli.htm` page which can be invoked using the Device API. 

- Use the pson variable **rpcCMD** to enter the command
- Press **Run** to invoke this command
- Open the **rpcRSP** pson variable, then press **Run** to display the result 

<img src="file://D:\Dropbox\Arduino\APPLICATIONS\SensGWY\doc-assets\Thinger API.JPG" width="600px" />

