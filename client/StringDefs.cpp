#include "stdinc.h"
#include "DCPlusPlus.h"
#include "ResourceManager.h"
string ResourceManager::strings[] = {
"Accepted Disconnects", 
"Accepted Timeouts", 
"Active", 
"Enabled / Search String", 
"&Add", 
"Add as source", 
"Add finished files to share instantly (if shared)", 
"Add To Favorites", 
"Added", 
"Automatic Directory Listing Search", 
"Destination Directory", 
"Discard", 
"Download Matches", 
"Enabled", 
"Full Path", 
"ADLSearch Properties", 
"Search String", 
"Max FileSize", 
"Min FileSize", 
"Search Type", 
"Size Type", 
"All", 
"All 3 users offline", 
"All 4 users offline", 
"All download slots taken", 
"All file slots taken", 
"All %d users offline", 
"Searching TTH alternates for: ", 
"-- WARNING --\r\n-- You are in active mode, but have activated passive search. --\r\n-- Don't use passive search if you have search results without this option --\r\n-- because you don't get search result from passive clients !!! --\r\n-- Turn it off in settings => StrongDC++ => Always use Passive Mode for Search --\r\n", 
"Any", 
"Autoscroll chat", 
"At least", 
"At most", 
"Audio", 
"Auto"	, 
"Auto connect / Name", 
"Auto grant slot / Nick", 
"Automatic", 
"Average/s: ", 
"Upload Speed", 
"AWAY", 
"Away mode off", 
"Away mode on: ", 
"B", 
"Background image", 
"Balloon popups", 
"Bitzi Lookup", 
"Segment Block Finished, waiting...", 
"Both users offline", 
"Browse...", 
"&Browse...", 
"Browse file list", 
"Bumped", 
"Cancel", 
"Clear chat", 
"Client", 
"Close", 
"Close connection", 
"Collapse All", 
"Active user", 
"User with fireball", 
"User is OP", 
"Passive user", 
"User with server", 
"Compressed", 
"Error during compression", 
"&Configure", 
"Configured Public Hub Lists", 
"&Connect", 
"Connect to all users", 
"Connected", 
"Connecting...", 
"Connecting (forced)...", 
"Connecting to ", 
"Connecting to server", 
"Connection", 
"Connection closed", 
"Connection error", 
"Connection timeout", 
"Continue Search", 
"Copy", 
"Copy All", 
"Copy description", 
"Copy email address", 
"Copy Exact Share", 
"Copy address to clipboard", 
"Copy IP", 
"Copy actual line", 
"Copy magnet link to clipboard", 
"Copy nick", 
"Copy Nick+IP", 
"Copy TAG", 
"Copy URL", 
"Corruption detected at position", 
"Could not open target file: ", 
"Count", 
"Country", 
"TTH Checked", 
"Hashing", 
"Current version", 
"Data retrieved", 
"Error during decompression", 
"Default", 
"Description", 
"Destination", 
"Directory", 
"Directory or directory name already exists", 
"Disable sounds", 
"Disabled", 
"Disconnect all users", 
"Disconnect user(s)", 
"Disconnected", 
"Disconnected user leaving the hub: ", 
"Document", 
"Done", 
"Don't begin new segment if overall file speed is over", 
"Don't remove /password before your password", 
"The temporary download directory cannot be shared", 
"Download", 
"Download failed: ", 
"Download finished, idle...", 
"Download queue", 
"Download starting...", 
"Download to...", 
"Download whole directory", 
"Download whole directory to...", 
"Download with priority...", 
"Downloaded", 
"Downloaded %s (%.01f%%) in %s", 
" downloaded from ", 
"Downloaded parts", 
"Downloading...", 
"Downloading public hub list...", 
"Downloading list...", 
"Down/Up:", 
"Duplicate source", 
"EB", 
"&Edit", 
"E-Mail", 
"Enable emoticons", 
"Enable multi-source", 
"Please enter a nickname in the settings dialog!", 
"Enter search string", 
"Error creating hash data file: ", 
"Error creating adc registry key", 
"Error creating dchub registry key", 
"Error creating magnet registry key", 
"Error hashing ", 
"Errors", 
"Exact Share", 
"Exact size", 
"Executable", 
"Expand All", 
"Expand search results", 
"Slot ratio", 
"Extra slots set", 
"Failed to shutdown!", 
"Join/part of favorite users showing off", 
"Join/part of favorite users showing on", 
"Favorite name", 
"Under what name you see the directory", 
"Favorite hub added", 
"Hub already exists as a favorite", 
"This hub is not a favorite hub", 
"Identification (leave blank for defaults)", 
"Favorite Hub Properties", 
"Favorite hub removed", 
"Favorite Hubs", 
"Favorite user added", 
"Favorite Users", 
"Favorite user online", 
"File", 
"Subtract list", 
"File list refresh finished", 
"File list refresh in progress, please wait for it to finish before trying to refresh again", 
"File list refresh initiated", 
"File not available", 
"File type", 
"A file with a different size already exists in the queue", 
"A file with diffent tth root already exists in the queue", 
"Filename", 
"Files", 
"files left", 
"files/h", 
"F&ilter", 
"Filtered", 
"Find", 
"Download finished: %s", 
"Finished Downloads", 
"Upload finished: %s To: %s"	, 
"Finished Uploads", 
"Folder", 
"Forbidden", 
"File with '$' cannot be downloaded and will not be shared: ", 
"Force attempt", 
"Send garbage on incomming connection (to avoid ISP P2P throttling)", 
"Send garbage on outgoing connection (to avoid ISP P2P throttling)", 
"GB", 
"Get file list", 
"Get user response", 
"Go to directory", 
"Grant extra slot (10 min)", 
"Grant extra slot (day)", 
"Grant extra slot (hour)", 
"Grant extra slot (week)", 
"Extra Slots", 
"Group search results by TTH", 
"Hash database", 
"Creating file index...", 
"Run in background", 
"Statistics", 
"Please wait while DC++ indexes your files (they won't be shared until they've been indexed)...", 
"Unable to read hash data file", 
"Hash database rebuilt", 
"Hashing failed: ", 
"Finished hashing: ", 
"Hibernate", 
"High", 
"Highest", 
"History", 
"Hits", 
"Hit Ratio: ", 
"Hits: ", 
"Hub", 
"Address", 
"Hub connected", 
"Hub disconnected", 
"Hublist", 
"Hub list downloaded...", 
"Edit the hublist", 
"Name", 
"Hub / Segments", 
"Users", 
"Hubs", 
"Chat double click on user action", 
"Cheating user found", 
"zero bytes real size", 
"Check File List", 
"Shared forbidden file : %s", 
"filelist was inflated %s times", 
"Mismatched share size - ", 
"Check user on join", 
", stated size = %[statedshareformat], real size = %[realshareformat]", 
"Don't check TTH on finish when more than 75% of file is verified", 
"Checking client...", 
"Verifying TTH...", 
"Choose folder", 
"Chunk overlapped by faster user", 
"Ignore TTH searches", 
"Ignore User", 
"Ignored message: ", 
"Hub address cannot be empty.", 
"Invalid file list name", 
"Invalid number of slots", 
"Invalid size", 
"Invalid target file (missing directory, check default download directory setting)", 
"Full tree does not match TTH root", 
"Ip: ", 
"Ip", 
"Items", 
"Join/part showing off", 
"Join/part showing on", 
"Joins: ", 
"kB", 
"kB/s", 
"kB/s (0 = disable)", 
"Kick user(s)", 
"Kick user(s) with filename", 
"A file of equal or larger size already exists at the target location", 
"Hub (last seen on if offline)", 
"Last change: ", 
"Time last seen", 
"Latest version", 
"File was corrupted, redownloading %s...", 
"left", 
"Left color", 
"Application \"%s\" caused an unhandled exception in StrongDC++. Please uninstall it, upgrade it or use an alternate product.", 
"Upload Limit", 
"Loading StrongDC++, please wait...", 
"Lock", 
"Log off", 
"Low", 
"Lowest", 
"Filename:", 
"File Hash:", 
"Do nothing", 
"Add this file to your download queue", 
"Do the same action next time without asking", 
"Start a search for this file", 
"File Size:", 
"A MAGNET link was given to DC++, but it didn't contain a valid file hash for use on the Direct Connect network.  No action will be taken.", 
"DC++ has detected a MAGNET link with a file hash that can be searched for on the Direct Connect network.  What would you like to do?", 
"MAGNET Link detected", 
"Download files from the Direct Connect network", 
"DC++", 
"URL:MAGNET URI", 
"Match queue", 
"Matched %d file(s)", 
"Max Hubs", 
"Max number of segments", 
"Max Size", 
"Max Users", 
"Max hubs", 
"Max users", 
"MB", 
"MBits/s"	, 
"MB/s", 
"About StrongDC++", 
"ADL Search", 
"Arrange icons", 
"Cascade", 
"CDM Debug Messages", 
"Close disconnected", 
"StrongDC++ Discussion forum", 
"&Download Queue\tCtrl+D", 
"E&xit", 
"&Favorite Hubs\tCtrl+F", 
"Favorite &Users\tCtrl+U", 
"&File", 
"&Recent Hubs", 
"Follow last redirec&t\tCtrl+T", 
"Indexing progress", 
"&Help", 
"GeoIP database update", 
"StrongDC++ Homepage", 
"Horizontal Tile", 
"Minimize &All", 
"Network Statistics", 
"&Notepad\tCtrl+N", 
"Open downloads directory", 
"Open file list...\tCtrl+L", 
"Open own list", 
"&Public Hubs\tCtrl+P", 
"&Quick Connect ...\tCtrl+Q", 
"&Reconnect\tCtrl+R", 
"Refresh file list\tCtrl+E", 
"Restore All", 
"&Search\tCtrl+S", 
"Search spy", 
"Settings...", 
"Show", 
"&Status bar\tCtrl+2", 
"&Toolbar\tCtrl+1", 
"T&ransfers\tCtrl+3", 
"&Transfers", 
"Get TTH for file...", 
"Vertical Tile", 
"&View", 
"&Window", 
"Menu bar", 
"Min Share", 
"Min Slots", 
"Minimum search interval", 
"Min share", 
"Min slots", 
"Minutes", 
"Mode", 
"Move/Rename", 
"Move &Down", 
"Move &Up", 
"My nick in mainchat", 
"NetLimiter was detected in your computer. It is frequently responsible for StrongDC++ crash, upload disconnecting and slow download speed. Please uninstall it and use an alternate product, or built-in limiter.", 
"Network Statistics", 
"&New...", 
"Remove user from queue, if speed is below", 
"Next", 
"Nick", 
"Your nick was already taken, please change to something else!", 
" (Nick unknown)", 
"No", 
"No directory specified", 
"Can't download from passive users when you're passive", 
"You're trying to download from yourself!", 
"No errors", 
"No free block", 
"Hub log not exist", 
"User log not exist", 
"No matches", 
"No needed part", 
"No slots available", 
"No users", 
"No users to download from", 
"Normal", 
"Notepad", 
"segment(s)", 
"Offline", 
"OK", 
"Online", 
"Only users with free slots", 
"Only results with TTH root", 
"Only where I'm op", 
"Open", 
"Open download page?", 
"Open folder", 
"Open hub log", 
"Open user log", 
"Parts: ", 
"Passive user", 
"Password", 
"Path", 
"Pause Search", 
"Paused", 
"PB", 
"Picture", 
"PK String", 
"Play", 
"Show popup on download failed", 
"Show popup on download finished", 
"Show popup on download begins", 
"Show popup on favorite user connected", 
"Show popup on hub connected", 
"Show popup on hub disconnected", 
"Show popup on cheating user found", 
"Show popup on new private message", 
"Show popup on private message", 
"Popup type", 
"Show popup on upload finished", 
"Port: ", 
"Power off", 
"Press the follow redirect button to connect to ", 
"Preview", 
"Previous folders", 
"Priority", 
"Private message", 
"Private message from ", 
"Transfer progressbar - background", 
"Transfer progressbar - decompressed part", 
"Queue progressbar - downloaded chunks", 
"Queue progressbar - running chunks", 
"Transfer progressbar - actual segment", 
"Queue progressbar - verified chunks", 
"Use oDC style of progressbar in Transfers", 
"&Properties", 
"Public Hubs", 
"Purge", 
"Enable queue updating in real time (use more CPU on large queue!!!)", 
"Quick Connect", 
"Rating", 
"Ratio", 
"Readd source", 
"Really exit?", 
"Really remove?", 
"Reason", 
"Reboot", 
"Recent Hubs", 
"Redirect request received to a hub that's already connected", 
"Redirect user(s)", 
"&Refresh", 
"Refresh user list", 
"Reliability", 
"&Remove", 
"Remove all", 
"Remove extra slot", 
"Remove user from queue", 
"Remove offline users", 
"Remove source", 
" renamed to ", 
"Report user", 
"Report auto search for alternates in status bar", 
"Retrieving data", 
"Right color", 
"Rollback inconsistency, existing file does not match the one being downloaded", 
"Running...", 
"Search", 
"Search for", 
"Search for alternates", 
"Search for file", 
"Search in results", 
"Search options", 
"Search spam detected from ", 
"Search Spy", 
"Search String", 
"Searching for ", 
"Seconds", 
"Request to seek beyond the end of data", 
"Segment", 
"Segments", 
"Select all", 
"Select user in list", 
"Send private message", 
"Send public message", 
"Separator", 
"Server", 
"Set priority", 
"Use Alternate Limiting from", 
"Back color", 
"Black & white", 
"Bold Authors of messages in chat", 
"Remove diacritic marks from Czech chars", 
"Default styles", 
"Enable slow downloads disconnecting", 
"Download Speed", 
"Enable Transfer Rate Limiting", 
"Error color", 
"And whole file speed exceeds", 
"Disconnect download if speed is below", 
"Install Magnet URI handler on startup", 
"And file size is more than", 
"* If upload limit is set, download limit is max 7 x upload limit !!!", 
"* Small Uploads Slots => Slots for filelist and Small Files.", 
"* Minimal upload limit is set to 5x(slots + 1) !!!", 
"Always use passive mode for Search (Use only if you know what this doing !!!)", 
"Lines from log on new PM", 
"Preview", 
"Private message sound", 
"Progressbar colors", 
"Progressbar text colors", 
"Remove Forbidden (Unfinished Kazaa, Win MX, GetRight, eMule, StrongDC++)", 
"Secondary Transfer Rate Limiting", 
"Disconnecting slow downloads", 
"Small file size", 
"Small Upload Slots", 
"Available styles", 
"Text color", 
"Text style", 
"More than", 
"to", 
"Transfer Rate Limiting", 
"Upload", 
"Upload Speed", 
"Winamp", 
"Settings", 
"&Add folder", 
"Break on first ADLSearch match", 
"Advanced", 
"Advanced resume using TTH", 
"Show share-checked users", 
"Suppress main chat", 
"Experts only", 
"Use antifragmentation method for downloads", 
"Appearance", 
"Arguments", 
"Auto-away on minimize (and back on restore)", 
"When segmented downloading only (more than 2 active sources)", 
"Automatically follow redirects", 
"Automatically disconnect users who leave the hub", 
"Auto-open at startup", 
"Use Auto Priority by default", 
"Auto refresh time", 
"Automatically search for alternative TTH source every", 
"Automatically match queue for auto search hits", 
"Auto-search limit"	, 
"File Preview", 
"Enable bad software detection", 
"Bind address", 
"Network interface for all connections", 
"Tab bolding on content change", 
"Connection type (like old DC++)", 
"Default (like plain DC++)", 
"Security Certificates", 
"Clear search box after each search", 
"Clients", 
"Search Alternate", 
"Bad client", 
"Bad filelist/fakeshare", 
"Client checked", 
"Favorite", 
"File list checked", 
"Full checked", 
"Ignored", 
"Normal", 
"Has reserved slot", 
"Other colors", 
"Command", 
"Enable safe and compressed transfers", 
"Configure Public Hub Lists", 
"Confirm dialog options", 
"Confirm application exit", 
"Confirm favorite hub removal", 
"Confirm item removal in download queue", 
"StrongDC++", 
"Highest Priority Extra Download Slots", 
"Debug commands", 
"Default away message", 
"Direct connection", 
"Directories", 
"Display cheats in main chat", 
"Don't download files already in share", 
"Default download directory", 
"Limits", 
"Downloads", 
"Maximum simultaneous downloads (0 = infinite)", 
"No new downloads if speed exceeds (kB/s, 0 = disable)", 
"Automatically expand folders in Queue", 
"Extensions", 
"External / WAN IP", 
"Fake detector", 
"Only show joins / parts for favorite users", 
"Favorite download directories", 
"Favorites", 
"Filename", 
"Maximum simultaneous files (0 = infinite)", 
"Filter kick and NMDC debug messages", 
"Firewall with manual port forwarding", 
"Firewall (passive, worst case)", 
"Firewall with UPnP (WinXP+ only)", 
"Format", 
"General", 
"Get User Country", 
"Accept custom user commands from hub", 
"&Change", 
"Ignore messages from users that are not online (effective against bots)", 
"Incoming connection settings (see Help/FAQ if unsure)", 
"Don't delete file lists when exiting", 
"Language file", 
"Limits", 
"Log directory", 
"Log downloads", 
"Log filelist transfers", 
"Log main chat", 
"Log private chat", 
"Log status messages", 
"Log system messages", 
"Log uploads", 
"Logging", 
"Logs", 
"Max compression level", 
"Max hash speed", 
"Max tab rows", 
"Minimize at program startup", 
"Minimize to tray", 
"Mouse Over", 
"Name", 
"Connection settings", 
"Don't send the away message to bots", 
"Nominal bandwidth", 
"Normal", 
"timeout", 
"Note; Files appear in the share only after they've been hashed!", 
"Open new window when using /join", 
"Progressbar colors", 
"Options", 
"Outgoing connection settings", 
"Don't allow hub/UPnP to override", 
"Personal Information", 
"Make an annoying sound every time a private message is received", 
"Make an annoying sound when a private message window is opened", 
"PM history", 
"Open new file list windows in the background", 
"Open new private message windows in the background", 
"Popup messages from users that are not online (if not ignoring, messages go to main chat if enabled)", 
"Popup private messages", 
"Ports", 
"High prio max size", 
"Highest prio max size", 
"Low prio max size", 
"Set lowest prio for newly added files larger than Low prio size", 
"Normal prio max size", 
"Public Hubs list", 
"HTTP Proxy (for hublist only)", 
"Public Hubs list URL", 
"Queue", 
"Rename", 
"Note; most of these options require that you restart StrongDC++", 
"Reset", 
"Rollback", 
"Max sources for match queue", 
"Search history", 
"Segment Downloading", 
"Select color", 
"&Text style", 
"&Window color", 
"Send unknown /commands to the hub", 
"Share hidden files", 
"Total size:", 
"Shared directories", 
"Show InfoTips in lists", 
"Show joins / parts in chat by default", 
"Show progress bars for transfers (uses some CPU)", 
"Skip zero-byte files", 
"SOCKS5", 
"Socks IP", 
"Port", 
"Use SOCKS5 server to resolve host names", 
"Login", 
"Sounds", 
"Sounds", 
"Note; because of changing download speeds, this is not 100% accurate...", 
"View status messages in main chat", 
"TCP Port", 
"Colors & Fonts", 
"Show timestamps in chat by default", 
"Set timestamps", 
"Toggle window when selecting an active tab", 
"Toolbar", 
"Add -->", 
"Toolbar Images", 
"<-- Remove", 
"UDP Port", 
"Unfinished downloads directory", 
"Line speed (upload)", 
"Sharing", 
"Automatically open an extra slot if speed is below (0 = disable)", 
"Upload slots", 
"Install URL handler on startup (to handle dchub:// links)", 
"Use CTRL for line history", 
"Use file extension for Download to in search", 
"Use OEM monospaced font for viewing text files", 
"Use old sharing user interface", 
"Use SSL when remote client supports it", 
"Use system icons when browsing files (slows browsing down a bit)", 
"Use vertical view by default", 
"User List Colors", 
"User Commands", 
"User Menu Items", 
"Enable Webserver", 
"Windows", 
"Window options", 
"Write buffer size", 
"Override system colors", 
"Override system colors", 
"Shared", 
"Shared Files", 
"Display popup in away mode only", 
"Display popup when minimized only", 
"Show speed", 
"Shutdown computer", 
"Shutdown action", 
"Shutdown sequence deactivated...", 
"Shutdown sequence activated...", 
"Size", 
"Exactly", 
"Min Size", 
"New virtual name matches old name, skipping...", 
"Slot granted", 
"Slots", 
"Slots set", 
"Slow user", 
"Small file size set", 
"Socks server authentication failed (bad login / password?)", 
"The socks server doesn't support login / password authentication", 
"The socks server failed establish a connection", 
"The socks server requires authentication", 
"Failed to set up the socks server for UDP relay (check socks address and port)", 
"Download begins", 
"Download finished", 
"Unhandled Exception", 
"Faker found", 
"Alternate source added", 
"File is corrupted", 
"Typing notification", 
"Upload finished", 
"Source Type", 
"Specify a search string", 
"Specify a server to connect to", 
"Speed", 
"EXPERIMENTAL: Try to switch to faster user when no free block", 
"Status", 
"Stored password sent...", 
"String not found: ", 
"Supports", 
"Suspend", 
"Tabs on top", 
"Tag", 
"Target filename too long", 
"TB", 
"Unable to open TCP port. File transfers will not work correctly until you change settings or turn off any application that might be using the TCP port", 
"Percent fake share accepted", 
"Manual settings of number of segments", 
"Time", 
"Time left", 
"\r\n- %a - Abbreviated weekday name\r\n- %A - Full weekday name\r\n- %b - Abbreviated month name\r\n- %B - Full month name\r\n- %c - Date and time representation appropriate for locale\r\n- %d - Day of month as decimal number (01 – 31)\r\n- %H - Hour in 24-hour format (00 – 23)\r\n- %I - Hour in 12-hour format (01 – 12)\r\n- %j - Day of year as decimal number (001 – 366)\r\n- %m - Month as decimal number (01 – 12)\r\n- %M - Minute as decimal number (00 – 59)\r\n- %p - Current locale's A.M./P.M. indicator for 12-hour clock\r\n- %S - Second as decimal number (00 – 59)\r\n- %U - Week of year as decimal number, with Sunday as first day of week (00 – 53)\r\n- %w - Weekday as decimal number (0 – 6; Sunday is 0)\r\n- %W - Week of year as decimal number, with Monday as first day of week (00 – 53)\r\n- %x - Date representation for current locale\r\n- %X - Time representation for current locale\r\n- %y - Year without century, as decimal number (00 – 99)\r\n- %Y - Year with century, as decimal number\r\n- %z, %Z - Either the time-zone name or time zone abbreviation, depending on registry settings; no characters if time zone is unknown\r\n- %% - Percent sign\r\n\r\nDefault: %H:%M:%S", 
"Timestamps Help", 
"Timestamps disabled", 
"Timestamps enabled", 
"More data was sent than was expected", 
"Total: ", 
"Transferlist double click action", 
"Transferred", 
"A file with the same hash already exists in your share", 
"TTH Inconsistency", 
"TTH Root", 
"Two colors", 
"Type", 
"Unable to create thread", 
"Unable to rename ", 
"Unignore User", 
"Unknown", 
"Unknown command: ", 
"Unknown error: 0x%x", 
"Update check", 
"Upload finished, idle...", 
"Upload starting...", 
"Uploaded %s (%.01f%%) in %s", 
" uploaded to ", 
"Failed to create port mappings. Please set up your NAT yourself.", 
"Failed to get external IP via  UPnP. Please set it yourself.", 
"Failed to remove port mappings", 
"User", 
"Command", 
"Context", 
"Filelist Menu", 
"Hub IP / DNS (empty = all, 'op' = where operator)", 
"Hub Menu", 
"Chat", 
"Send once per nick", 
"Parameters", 
"PM", 
"Text sent to hub", 
"Raw", 
"Search Menu", 
"To", 
"Command Type", 
"User Menu", 
"Create / Modify Command", 
"User Description", 
"User offline", 
"Running... (user online)", 
"User went offline", 
"User went online", 
"Info User", 
"Userlist Icons", 
"Userlist double click action", 
"Users", 
"Running... (%d of %d users online)", 
"Version", 
"Video", 
"View as text", 
"Virtual name", 
"Virtual directory name already exists", 
"Name under which the others see the directory", 
"Waiting...", 
"Waiting for %i seconds before searching...", 
"Waiting time", 
"Waiting to retry...", 
"Waiting (User online)", 
"Waiting Users", 
"Waiting (%d of %d users online)", 
"Webserver", 
"What's &this?", 
"Whois ", 
"/winamp - Works with 1.x, 2.x, 5.x (no WinAmp 3 support)\r\n- %[version]	Numerical Version (ex: 2.91)\r\n- %[state]	Playing state (ex: stopped/paused/playing)\r\n- %[title]		Window title from Winamp - if you want to change this for mp3s, Winamp > Pref > Input > MPEG > Title\r\n- %[rawtitle]	Window title from Winamp (if %[title] not working propertly)\r\n- %[percent]	Percentage (ex. 40%)\r\n- %[length]	Length in minutes:seconds (ex: 04:09)\r\n- %[elapsed]	Time elapsed in minutes:seconds (ex. 03:51)\r\n- %[bar]		ASCII progress bar, 10 characters wide no including brackets (ex. [----|-----])\r\n- %[bitrate]	Bitrate (ex. 128kbps)\r\n- %[sample]	Sample frequency (ex. 22kHz)\r\n- %[channels]	Number of channels (ex. stereo / mono)\r\nEmpty = Default String -> winamp(%[version]) %[state](%[title]) stats(%[percent] of %[length] %[bar])", 
"Winamp Help", 
"Yes", 
"ZoneAlarm was detected in your computer.  It is frequently responsible for corrupted downloads and is the cause of many \"rollback inconsistency\" errors.  Please uninstall it and use an alternate product.  ", 
};
string ResourceManager::names[] = {
"AcceptedDisconnects", 
"AcceptedTimeouts", 
"Active", 
"ActiveSearchString", 
"Add", 
"AddAsSource", 
"AddFinishedInstantly", 
"AddToFavorites", 
"Added", 
"AdlSearch", 
"AdlsDestination", 
"AdlsDiscard", 
"AdlsDownload", 
"AdlsEnabled", 
"AdlsFullPath", 
"AdlsProperties", 
"AdlsSearchString", 
"AdlsSizeMax", 
"AdlsSizeMin", 
"AdlsType", 
"AdlsUnits", 
"All", 
"All3UsersOffline", 
"All4UsersOffline", 
"AllDownloadSlotsTaken", 
"AllFileSlotsTaken", 
"AllUsersOffline", 
"AlternatesSend", 
"AntiPassiveSearch", 
"Any", 
"AscrollChat", 
"AtLeast", 
"AtMost", 
"Audio", 
"Auto", 
"AutoConnect", 
"AutoGrant", 
"Automatic", 
"Average", 
"AverageUpload", 
"Away", 
"AwayModeOff", 
"AwayModeOn", 
"B", 
"BackgroundImage", 
"BalloonPopups", 
"BitziLookup", 
"BlockFinished", 
"BothUsersOffline", 
"Browse", 
"BrowseAccel", 
"BrowseFileList", 
"Bumped", 
"Cancel", 
"Clear", 
"Clientid", 
"Close", 
"CloseConnection", 
"CollapseAll", 
"ColorActive", 
"ColorFast", 
"ColorOp", 
"ColorPasive", 
"ColorServer", 
"Compressed", 
"CompressionError", 
"Configure", 
"ConfiguredHubLists", 
"Connect", 
"ConnectAll", 
"Connected", 
"Connecting", 
"ConnectingForced", 
"ConnectingTo", 
"ConnectingToServer", 
"Connection", 
"ConnectionClosed", 
"ConnectionError", 
"ConnectionTimeout", 
"ContinueSearch", 
"Copy", 
"CopyAll", 
"CopyDescription", 
"CopyEmailAddress", 
"CopyExactShare", 
"CopyHub", 
"CopyIp", 
"CopyLine", 
"CopyMagnetLink", 
"CopyNick", 
"CopyNickIp", 
"CopyTag", 
"CopyUrl", 
"CorruptionDetected", 
"CouldNotOpenTargetFile", 
"Count", 
"Country", 
"CrcChecked", 
"CreatingHash", 
"CurrentVersion", 
"DataRetrieved", 
"DecompressionError", 
"Default", 
"Description", 
"Destination", 
"Directory", 
"DirectoryAddError", 
"DisableSounds", 
"Disabled", 
"DisconnectAll", 
"DisconnectUser", 
"Disconnected", 
"DisconnectedUser", 
"Document", 
"Done", 
"DontAddSegmentText", 
"DontRemoveSlashPassword", 
"DontShareTempDirectory", 
"Download", 
"DownloadFailed", 
"DownloadFinishedIdle", 
"DownloadQueue", 
"DownloadStarting", 
"DownloadTo", 
"DownloadWholeDir", 
"DownloadWholeDirTo", 
"DownloadWithPriority", 
"Downloaded", 
"DownloadedBytes", 
"DownloadedFrom", 
"DownloadedParts", 
"Downloading", 
"DownloadingHubList", 
"DownloadingList", 
"Du", 
"DuplicateSource", 
"Eb", 
"EditAccel", 
"Email", 
"EnableEmoticons", 
"EnableMultiSource", 
"EnterNick", 
"EnterSearchString", 
"ErrorCreatingHashDataFile", 
"ErrorCreatingRegistryKeyAdc", 
"ErrorCreatingRegistryKeyDchub", 
"ErrorCreatingRegistryKeyMagnet", 
"ErrorHashing", 
"Errors", 
"ExactShared", 
"ExactSize", 
"Executable", 
"ExpandAll", 
"ExpandedResults", 
"ExtraHubSlots", 
"ExtraSlotsSet", 
"FailedToShutdown", 
"FavJoinShowingOff", 
"FavJoinShowingOn", 
"FavoriteDirName", 
"FavoriteDirNameLong", 
"FavoriteHubAdded", 
"FavoriteHubAlreadyExists", 
"FavoriteHubDoesNotExist", 
"FavoriteHubIdentity", 
"FavoriteHubProperties", 
"FavoriteHubRemoved", 
"FavoriteHubs", 
"FavoriteUserAdded", 
"FavoriteUsers", 
"FavuserOnline", 
"File", 
"FileListDiff", 
"FileListRefreshFinished", 
"FileListRefreshInProgress", 
"FileListRefreshInitiated", 
"FileNotAvailable", 
"FileType", 
"FileWithDifferentSize", 
"FileWithDifferentTth", 
"Filename", 
"Files", 
"FilesLeft", 
"FilesPerHour", 
"Filter", 
"Filtered", 
"Find", 
"FinishedDownload", 
"FinishedDownloads", 
"FinishedUpload", 
"FinishedUploads", 
"Folder", 
"Forbidden", 
"ForbiddenDollarFile", 
"ForceAttempt", 
"GarbageIncoming", 
"GarbageOutgoing", 
"Gb", 
"GetFileList", 
"GetUserResponses", 
"GoToDirectory", 
"GrantExtraSlot", 
"GrantExtraSlotDay", 
"GrantExtraSlotHour", 
"GrantExtraSlotWeek", 
"GrantSlotsMenu", 
"GroupSearchResults", 
"HashDatabase", 
"HashProgress", 
"HashProgressBackground", 
"HashProgressStats", 
"HashProgressText", 
"HashReadFailed", 
"HashRebuilt", 
"HashingFailed", 
"HashingFinished", 
"Hibernate", 
"High", 
"Highest", 
"History", 
"HitCount", 
"HitRatio", 
"Hits", 
"Hub", 
"HubAddress", 
"HubConnected", 
"HubDisconnected", 
"HubList", 
"HubListDownloaded", 
"HubListEdit", 
"HubName", 
"HubSegments", 
"HubUsers", 
"Hubs", 
"Chatdblclickaction", 
"CheatingUser", 
"Check0byteShare", 
"CheckFilelist", 
"CheckForbidden", 
"CheckInflated", 
"CheckMismatchedShareSize", 
"CheckOnConnect", 
"CheckShowRealShare", 
"CheckUnverifiedOnly", 
"CheckingClient", 
"CheckingTth", 
"ChooseFolder", 
"ChunkOverlapped", 
"IgnoreTthSearches", 
"IgnoreUser", 
"IgnoredMessage", 
"IncompleteFavHub", 
"InvalidListname", 
"InvalidNumberOfSlots", 
"InvalidSize", 
"InvalidTargetFile", 
"InvalidTree", 
"Ip", 
"IpBare", 
"Items", 
"JoinShowingOff", 
"JoinShowingOn", 
"Joins", 
"Kb", 
"Kbps", 
"KbpsDisable", 
"KickUser", 
"KickUserFile", 
"LargerTargetFileExists", 
"LastHub", 
"LastChange", 
"LastSeen", 
"LatestVersion", 
"LeafCorrupted", 
"Left", 
"LeftColor", 
"LibCrash", 
"Limit", 
"Loading", 
"Lock", 
"LogOff", 
"Low", 
"Lowest", 
"MagnetDlgFile", 
"MagnetDlgHash", 
"MagnetDlgNothing", 
"MagnetDlgQueue", 
"MagnetDlgRemember", 
"MagnetDlgSearch", 
"MagnetDlgSize", 
"MagnetDlgTextBad", 
"MagnetDlgTextGood", 
"MagnetDlgTitle", 
"MagnetHandlerDesc", 
"MagnetHandlerRoot", 
"MagnetShellDesc", 
"MatchQueue", 
"MatchedFiles", 
"MaxHubs", 
"MaxSegmentsNumber", 
"MaxSize", 
"MaxUsers", 
"Maxhubs", 
"Maxusers", 
"Mb", 
"Mbitsps", 
"Mbps", 
"MenuAbout", 
"MenuAdlSearch", 
"MenuArrange", 
"MenuCascade", 
"MenuCdmdebugMessages", 
"MenuCloseDisconnected", 
"MenuDiscuss", 
"MenuDownloadQueue", 
"MenuExit", 
"MenuFavoriteHubs", 
"MenuFavoriteUsers", 
"MenuFile", 
"MenuFileRecentHubs", 
"MenuFollowRedirect", 
"MenuHashProgress", 
"MenuHelp", 
"MenuHelpGeoipfile", 
"MenuHomepage", 
"MenuHorizontalTile", 
"MenuMinimizeAll", 
"MenuNetworkStatistics", 
"MenuNotepad", 
"MenuOpenDownloadsDir", 
"MenuOpenFileList", 
"MenuOpenOwnList", 
"MenuPublicHubs", 
"MenuQuickConnect", 
"MenuReconnect", 
"MenuRefreshFileList", 
"MenuRestoreAll", 
"MenuSearch", 
"MenuSearchSpy", 
"MenuSettings", 
"MenuShow", 
"MenuStatusBar", 
"MenuToolbar", 
"MenuTransferView", 
"MenuTransfers", 
"MenuTth", 
"MenuVerticalTile", 
"MenuView", 
"MenuWindow", 
"Menubar", 
"MinShare", 
"MinSlots", 
"MinimumSearchInterval", 
"Minshare", 
"Minslots", 
"Minutes", 
"Mode", 
"Move", 
"MoveDown", 
"MoveUp", 
"MynickInChat", 
"NetlimiterWarning", 
"NetworkStatistics", 
"New", 
"NewDisconnect", 
"Next", 
"Nick", 
"NickTaken", 
"NickUnknown", 
"No", 
"NoDirectorySpecified", 
"NoDownloadsFromPassive", 
"NoDownloadsFromSelf", 
"NoErrors", 
"NoFreeBlock", 
"NoLogForHub", 
"NoLogForUser", 
"NoMatches", 
"NoNeededPart", 
"NoSlotsAvailable", 
"NoUsers", 
"NoUsersToDownloadFrom", 
"Normal", 
"Notepad", 
"NumberOfSegments", 
"Offline", 
"Ok", 
"Online", 
"OnlyFreeSlots", 
"OnlyTth", 
"OnlyWhereOp", 
"Open", 
"OpenDownloadPage", 
"OpenFolder", 
"OpenHubLog", 
"OpenUserLog", 
"Parts", 
"PassiveUser", 
"Password", 
"Path", 
"PauseSearch", 
"Paused", 
"Pb", 
"Picture", 
"Pk", 
"Play", 
"PopupDownloadFailed", 
"PopupDownloadFinished", 
"PopupDownloadStart", 
"PopupFavoriteConnected", 
"PopupHubConnected", 
"PopupHubDisconnected", 
"PopupCheatingUser", 
"PopupNewPm", 
"PopupPm", 
"PopupType", 
"PopupUploadFinished", 
"Port", 
"PowerOff", 
"PressFollow", 
"PreviewMenu", 
"PreviousFolders", 
"Priority", 
"PrivateMessage", 
"PrivateMessageFrom", 
"ProgressBack", 
"ProgressCompress", 
"ProgressDownloaded", 
"ProgressRunning", 
"ProgressSegment", 
"ProgressVerified", 
"ProgressbarOdcStyle", 
"Properties", 
"PublicHubs", 
"Purge", 
"QueueUpdating", 
"QuickConnect", 
"Rating", 
"Ratio", 
"ReaddSource", 
"ReallyExit", 
"ReallyRemove", 
"Reason", 
"Reboot", 
"RecentHubs", 
"RedirectAlreadyConnected", 
"RedirectUser", 
"Refresh", 
"RefreshUserList", 
"Reliability", 
"Remove", 
"RemoveAll", 
"RemoveExtraSlot", 
"RemoveFromAll", 
"RemoveOffline", 
"RemoveSource", 
"RenamedTo", 
"Report", 
"ReportAlternates", 
"RetrievingData", 
"RightColor", 
"RollbackInconsistency", 
"Running", 
"Search", 
"SearchFor", 
"SearchForAlternates", 
"SearchForFile", 
"SearchInResults", 
"SearchOptions", 
"SearchSpamFrom", 
"SearchSpy", 
"SearchString", 
"SearchingFor", 
"Seconds", 
"SeekBeyondEnd", 
"Segment", 
"Segments", 
"SelectAll", 
"SelectUserList", 
"SendPrivateMessage", 
"SendPublicMessage", 
"Separator", 
"Server", 
"SetPriority", 
"SetczdcAlternateLimiting", 
"SetczdcBackColor", 
"SetczdcBlackWhite", 
"SetczdcBold", 
"SetczdcCzcharsDisable", 
"SetczdcDefaultStyle", 
"SetczdcDisconnectingEnable", 
"SetczdcDownloadSpeed", 
"SetczdcEnableLimiting", 
"SetczdcErrorColor", 
"SetczdcHDownSpeed", 
"SetczdcIDownSpeed", 
"SetczdcMagnetUriHandler", 
"SetczdcMinFileSize", 
"SetczdcNoteDownload", 
"SetczdcNoteSmallUp", 
"SetczdcNoteUpload", 
"SetczdcPassiveSearch", 
"SetczdcPmLines", 
"SetczdcPreview", 
"SetczdcPrivateSound", 
"SetczdcProgressbarColors", 
"SetczdcProgressbarText", 
"SetczdcRemoveForbidden", 
"SetczdcSecondaryLimiting", 
"SetczdcSlowDisconnect", 
"SetczdcSmallFiles", 
"SetczdcSmallUpSlots", 
"SetczdcStyles", 
"SetczdcTextColor", 
"SetczdcTextStyle", 
"SetczdcTimeDown", 
"SetczdcTo", 
"SetczdcTransferLimiting", 
"SetczdcUpload", 
"SetczdcUploadSpeed", 
"SetczdcWinamp", 
"Settings", 
"SettingsAddFolder", 
"SettingsAdlsBreakOnFirst", 
"SettingsAdvanced", 
"SettingsAdvancedResume", 
"SettingsAdvancedShowShareCheckedUsers", 
"SettingsAdvancedSuppressMainChat", 
"SettingsAdvanced3", 
"SettingsAntiFrag", 
"SettingsAppearance", 
"SettingsArgument", 
"SettingsAutoAway", 
"SettingsAutoDropSegmentedSource", 
"SettingsAutoFollow", 
"SettingsAutoKick", 
"SettingsAutoOpen", 
"SettingsAutoPriorityDefault", 
"SettingsAutoRefreshTime", 
"SettingsAutoSearch", 
"SettingsAutoSearchAutoMatch", 
"SettingsAutoSearchLimit", 
"SettingsAvipreview", 
"SettingsBadSoftware", 
"SettingsBindAddress", 
"SettingsBindAddressHelp", 
"SettingsBoldOptions", 
"SettingsBwboth", 
"SettingsBwsingle", 
"SettingsCertificates", 
"SettingsClearSearch", 
"SettingsClients", 
"SettingsColorAlternate", 
"SettingsColorBadClient", 
"SettingsColorBadFilelist", 
"SettingsColorClientChecked", 
"SettingsColorFavorite", 
"SettingsColorFilelistChecked", 
"SettingsColorFullChecked", 
"SettingsColorIgnored", 
"SettingsColorNormal", 
"SettingsColorReserved", 
"SettingsColors", 
"SettingsCommand", 
"SettingsCompressTransfers", 
"SettingsConfigureHubLists", 
"SettingsConfirmDialogOptions", 
"SettingsConfirmExit", 
"SettingsConfirmHubRemoval", 
"SettingsConfirmItemRemoval", 
"SettingsCzdc", 
"SettingsCzdcExtraDownloads", 
"SettingsDebugCommands", 
"SettingsDefaultAwayMsg", 
"SettingsDirect", 
"SettingsDirectories", 
"SettingsDisplayCheatsInMainChat", 
"SettingsDontDlAlreadyShared", 
"SettingsDownloadDirectory", 
"SettingsDownloadLimits", 
"SettingsDownloads", 
"SettingsDownloadsMax", 
"SettingsDownloadsSpeedPause", 
"SettingsExpandQueue", 
"SettingsExtensions", 
"SettingsExternalIp", 
"SettingsFakedetect", 
"SettingsFavShowJoins", 
"SettingsFavoriteDirs", 
"SettingsFavoriteDirsPage", 
"SettingsFileName", 
"SettingsFilesMax", 
"SettingsFilterMessages", 
"SettingsFirewallNat", 
"SettingsFirewallPassive", 
"SettingsFirewallUpnp", 
"SettingsFormat", 
"SettingsGeneral", 
"SettingsGetUserCountry", 
"SettingsHubUserCommands", 
"SettingsChange", 
"SettingsIgnoreOffline", 
"SettingsIncoming", 
"SettingsKeepLists", 
"SettingsLanguageFile", 
"SettingsLimit", 
"SettingsLogDir", 
"SettingsLogDownloads", 
"SettingsLogFilelistTransfers", 
"SettingsLogMainChat", 
"SettingsLogPrivateChat", 
"SettingsLogStatusMessages", 
"SettingsLogSystemMessages", 
"SettingsLogUploads", 
"SettingsLogging", 
"SettingsLogs", 
"SettingsMaxCompress", 
"SettingsMaxHashSpeed", 
"SettingsMaxTabRows", 
"SettingsMinimizeOnStartup", 
"SettingsMinimizeTray", 
"SettingsMouseOver", 
"SettingsName", 
"SettingsNetwork", 
"SettingsNoAwaymsgToBots", 
"SettingsNominalBandwidth", 
"SettingsNormal", 
"SettingsOdcShutdowntimeout", 
"SettingsOnlyHashed", 
"SettingsOpenNewWindow", 
"SettingsOperacolors", 
"SettingsOptions", 
"SettingsOutgoing", 
"SettingsOverride", 
"SettingsPersonalInformation", 
"SettingsPmBeep", 
"SettingsPmBeepOpen", 
"SettingsPmHistory", 
"SettingsPopunderFilelist", 
"SettingsPopunderPm", 
"SettingsPopupOffline", 
"SettingsPopupPms", 
"SettingsPorts", 
"SettingsPrioHigh", 
"SettingsPrioHighest", 
"SettingsPrioLow", 
"SettingsPrioLowest", 
"SettingsPrioNormal", 
"SettingsPublicHubList", 
"SettingsPublicHubListHttpProxy", 
"SettingsPublicHubListUrl", 
"SettingsQueue", 
"SettingsRenameFolder", 
"SettingsRequiresRestart", 
"SettingsReset", 
"SettingsRollback", 
"SettingsSbMaxSources", 
"SettingsSearchHistory", 
"SettingsSegment", 
"SettingsSelectColor", 
"SettingsSelectTextFace", 
"SettingsSelectWindowColor", 
"SettingsSendUnknownCommands", 
"SettingsShareHidden", 
"SettingsShareSize", 
"SettingsSharedDirectories", 
"SettingsShowInfoTips", 
"SettingsShowJoins", 
"SettingsShowProgressBars", 
"SettingsSkipZeroByte", 
"SettingsSocks5", 
"SettingsSocks5Ip", 
"SettingsSocks5Port", 
"SettingsSocks5Resolve", 
"SettingsSocks5Username", 
"SettingsSound", 
"SettingsSounds", 
"SettingsSpeedsNotAccurate", 
"SettingsStatusInChat", 
"SettingsTcpPort", 
"SettingsTextStyles", 
"SettingsTimeStamps", 
"SettingsTimeStampsFormat", 
"SettingsToggleActiveWindow", 
"SettingsToolbar", 
"SettingsToolbarAdd", 
"SettingsToolbarImage", 
"SettingsToolbarRemove", 
"SettingsUdpPort", 
"SettingsUnfinishedDownloadDirectory", 
"SettingsUploadLineSpeed", 
"SettingsUploads", 
"SettingsUploadsMinSpeed", 
"SettingsUploadsSlots", 
"SettingsUrlHandler", 
"SettingsUseCtrlForLineHistory", 
"SettingsUseExtensionDownto", 
"SettingsUseOemMonofont", 
"SettingsUseOldSharingUi", 
"SettingsUseSsl", 
"SettingsUseSystemIcons", 
"SettingsUseVerticalView", 
"SettingsUserColors", 
"SettingsUserCommands", 
"SettingsUserMenu", 
"SettingsWebserver", 
"SettingsWindows", 
"SettingsWindowsOptions", 
"SettingsWriteBuffer", 
"SettingsZdcProgressOverride", 
"SettingsZdcProgressOverride2", 
"Shared", 
"SharedFiles", 
"ShowPopupAway", 
"ShowPopupMinimized", 
"ShowSpeed", 
"Shutdown", 
"ShutdownAction", 
"ShutdownOff", 
"ShutdownOn", 
"Size", 
"SizeExact", 
"SizeMin", 
"SkipRename", 
"SlotGranted", 
"Slots", 
"SlotsSet", 
"SlowUser", 
"SmallFileSizeSet", 
"SocksAuthFailed", 
"SocksAuthUnsupported", 
"SocksFailed", 
"SocksNeedsAuth", 
"SocksSetupError", 
"SoundDownloadBegins", 
"SoundDownloadFinished", 
"SoundException", 
"SoundFakerFound", 
"SoundSourceAdded", 
"SoundTthInvalid", 
"SoundTypingNotify", 
"SoundUploadFinished", 
"SourceType", 
"SpecifySearchString", 
"SpecifyServer", 
"Speed", 
"SpeedUsers", 
"Status", 
"StoredPasswordSent", 
"StringNotFound", 
"Supports", 
"Suspend", 
"TabsOnTop", 
"Tag", 
"TargetFilenameTooLong", 
"Tb", 
"TcpPortBusy", 
"TextFakepercent", 
"TextManual", 
"Time", 
"TimeLeft", 
"TimestampHelp", 
"TimestampHelpDesc", 
"TimestampsDisabled", 
"TimestampsEnabled", 
"TooMuchData", 
"Total", 
"Transferlistdblclickaction", 
"Transferred", 
"TthAlreadyShared", 
"TthInconsistency", 
"TthRoot", 
"TwoColors", 
"Type", 
"UnableToCreateThread", 
"UnableToRename", 
"UnignoreUser", 
"Unknown", 
"UnknownCommand", 
"UnknownError", 
"UpdateCheck", 
"UploadFinishedIdle", 
"UploadStarting", 
"UploadedBytes", 
"UploadedTo", 
"UpnpFailedToCreateMappings", 
"UpnpFailedToGetExternalIp", 
"UpnpFailedToRemoveMappings", 
"User", 
"UserCmdCommand", 
"UserCmdContext", 
"UserCmdFilelistMenu", 
"UserCmdHub", 
"UserCmdHubMenu", 
"UserCmdChat", 
"UserCmdOnce", 
"UserCmdParameters", 
"UserCmdPm", 
"UserCmdPreview", 
"UserCmdRaw", 
"UserCmdSearchMenu", 
"UserCmdTo", 
"UserCmdType", 
"UserCmdUserMenu", 
"UserCmdWindow", 
"UserDescription", 
"UserOffline", 
"UserOnline", 
"UserWentOffline", 
"UserWentOnline", 
"Userinfo", 
"UserlistIcons", 
"Userlistdblclickaction", 
"Users", 
"UsersOnline", 
"Version", 
"Video", 
"ViewAsText", 
"VirtualName", 
"VirtualNameExists", 
"VirtualNameLong", 
"Waiting", 
"WaitingFor", 
"WaitingTime", 
"WaitingToRetry", 
"WaitingUserOnline", 
"WaitingUsers", 
"WaitingUsersOnline", 
"Webserver", 
"WhatsThis", 
"WhoIs", 
"WinampHelp", 
"WinampHelpDesc", 
"Yes", 
"ZonealarmWarning", 
};
