# Steam API Parser for TF2 Statistics

This project uses the Steam Web API to fetch your TF2 player statistics, like the ones you are already able to view on Steam, however, it also includes some hidden statistics that you don't have easy access to, such as your playtime for each class in Mann vs. Machine (and many others like total damage dealt for each class!) or your total playtime on a number of official maps such as plr_hightower, pl_upward, etc.

Be aware that achievement stats stop counting once you have met the requirements for obtaining the achievement the stat pertains to.

For now, only a Linux binary release is provided (can be found as "build artifacts" under the Actions tab) until I am able to figure out how to get it to build on Windows/MSVC with GitHub Actions.

## Usage

Requirements:

- A `SteamID64` which looks like this: `76561198076611413`
  - You can find yours using sites like [SteamID I/O](https://steamid.io/)
- A Steam API key for your account which you can find (or generate if you do not have one) on this [Steam page](https://steamcommunity.com/dev/apikey)
  - **DO NOT SHARE** your API key with anybody!

Once you have a Steam ID and an API key, you may either run the program via the command line/terminal and use your Steam ID and API key as program arguments (i.e. `$ tf-steam-api-parser steamid64 apikey`) or you can just open it without specifying any arguments and it should manually ask you for your Steam ID and API key.

When it is done fetching the data and parsing it (it should be near instant), the output will be a Markdown file called `stats.md` which contains all TF2 statistics for the Steam account.
