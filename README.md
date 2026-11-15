# Single Race mode for NFS Hot Pursuit Remastered
A mod that adds customizable Single Race mode to the game, replacing the Wall menu.

# A long rant about how we got here
Ever since the original NFS Hot Pursuit was released in 2010, I always wondered why the developers never implemented customizable single player races. Up until then, that feature had been present in all Need for Speed games and most racing games to date. I always felt that this severely decreased the replay value of the game - because each time I fired up an event, it looked exactly the same. I was also frustrated that in order to take my favourite car for a spin, I would have to browse through the extensive event list, to find the one that 1) mercifully lets me use the car that I want, and 2) more or less has the settings that I fancied playing at the moment (mode, weather, time of day etc.)

As someone who was never interested in multiplayer, I discarded the game pretty quickly. I would instead go back to play the original six Need for Speed games (TNFS - NFSHP2), because there I could freely set up races to my liking, instead of having to work with whatever the developers thought was "cool" at that time.

When Hot Pursuit Remastered came about, I was hoping that Stellar Entertainment would add a proper Single Race mode, but of course, that did not happen.

Then I started experimenting with Claude, working on smaller mods for other games. This eventually led me to make the [Event Randomizer mod](https://github.com/HungryManticore/NFSHPR-EventRandomizer), which randomizes the opponents, time of day, and weather in the Career mode. Once I got that working correctly, I thought I would try my luck and see if implementing fully customizable single races into the game was possible.

This mod is the result of that 16-year-old frustration, and also proof that the game engine handles custom races just fine (except a minor cop spawning bug - see the **Known issues** section below). Which makes me think that Criterion/Stellar could have done it easily themselves; they chose not to.

# What you get
- 100+ tracks to choose from; both Career and Multiplayer routes are used. Tracks are based on the vanilla game routes - Race, Duel, Hot Pursuit, Gauntlet, Preview, Rapid Response, and Time Trials.
- You can play each track either as a Race, a racer Hot Pursuit, or a cop Hot Pursuit.
- You can choose time of day, weather, and difficulty level (Easy/Medium/Hard).
- You can pick the number of AI opponents: 1-7 in Race and cop Hot Pursuit. The racer Hot Pursuit events are limited to 7 bots total due to engine limitations, but you can freely choose between 1-6 racers and 1-6 cops as long as the total number does not exceed 7.
- You can select any car from any class; the AI racers and cops will be randomized, based on the class of your vehicle.
- You can customize your weapons. Two options are available: Custom, where you can choose whatever you want, and Classic Hot Pursuit inspired by NFS3, NFS High Stakes and NFS Hot Pursuit 2 where racers get no weapons, and cops get to use Helicopter, Roadblock, and Spike Strips.
- The Single Race event picker, as well as the race loading screen correctly display current Single Race settings.
- The Career mode remains untouched.

# Installation
Download the latest [release](../../releases). Unpack everything to the game's root directory.

# How to use
- Upon starting the game, you'll notice that the Wall menu has been replaced with Single Race.
- When you enter it, the game displays all Single Race tracks for you to choose, similar how the Career mode presents them.
- Press F5 on the keyboard or Select on your controller to bring up the race setup menu.
- You can switch between the Race Setup and Weapons tabs by highlighting them and pressing the arrow keys or left or right on your D-pad. You can also switch between them using LB/RB on your controller. Hit A / Enter to confirm your selection.
- Pick an event from the map and accept it. You will then proceed to the car selection screen. Accept your car to start the race.

# Screenshots
<img width="1920" height="1080" alt="Screenshot (57)" src="https://github.com/user-attachments/assets/0ce98009-52a0-4150-b98a-ad61afc5f26b" />
<img width="1920" height="1080" alt="Screenshot (58)" src="https://github.com/user-attachments/assets/becba3d5-cc8b-4ecd-8144-9e7a21f6612c" />
<img width="1920" height="1080" alt="Screenshot (59)" src="https://github.com/user-attachments/assets/7de4a6be-cd2f-4562-9078-a9e6c09b8a61" />
<img width="1920" height="1080" alt="Screenshot (60)" src="https://github.com/user-attachments/assets/00ab029f-506e-456b-8835-1282ebf6d37a" />
<img width="1920" height="1080" alt="Screenshot (61)" src="https://github.com/user-attachments/assets/65252f2c-2f35-4b9f-a2d3-e4340adcb59c" />

# Known issues and limitations
- **Rain does not work in certain conditions.** This is an issue in the vanilla game as well - there are certain zones/times of day where rain does not appear even if selected in Single Race. I have not yet found a proper fix.
- **Respawned cops sometimes appear in incorrect locations**, such as on top of guard rails. This is due to how the vanilla game calculates their spawn points.
- **Unknown compatibility with the [Event Randomizer mod](https://github.com/HungryManticore/NFSHPR-EventRandomizer).** They will likely conflict with one another, so it's best to avoid using both at the same time.
- **Multiplayer has not been tested**. In theory, the mod should not touch the online mode.

I will try to find the way to fix the cop spawning and the weather patterns, though I cannot make any promises at this stage.

# AI disclosure
This project was made with Claude; I tested it myself.
