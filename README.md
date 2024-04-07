# EEG-VR_Meditation
Development of a VR experience with EEG sensors

### Background explanation
Thanks to a partnership between my school ISART Digital, and the Tokyo University of Technology, I had the opportunity to do a 1 year research in Japan, Japan being my dream country that I long wanted to visit for an extended period of time.

For my research, I decided to explore about the usage of EEG to improve immersion sensation in VR.

### Technical environment
- Meta Quest 2 as VR headset
- Emotiv Epoc+ and Neurosky Mindwave Mobile 2 as EEG sensors
- Unreal Engine 5 (C++ & Blueprint), OpenViBE

### Research content
My research was divided in 4 blocs:
- Paper reading
- Device and software setup
- Experimenting and testing the EEG devices
- Exploring gameplay ideas and creation of a VR prototype using EEG Data


I learnt the basics of EEG, about existing EEG headsets, and about the current state of the art of EEG and how it is used in various fields, and more thoroughly about VR and Games.
This was done through reading scientific papers, exploring EEG manufacturer websites, forums, educational videos...

I then configured the equipment and the software, and setted up communication between the EEG headset and the different softwares to enable the game engine to receive EEG data.

When the basic setup was done, I tested the EEG headsets, Emotiv Epoc first, and later the Neurosky Mindwave.

After a first successful attempt of using the EEG data in unreal engine, with a visual orb changing its color depending on the user's mental state, I looked into some ideas to develop a prototype using EEG values.

### Prototype
I decided to create a meditation prototype using the meditative state provided by Neurosky Mindwave.

My goal was to create a relaxing experience in the form of a contemplative game, first by ascending more and more as long as the user is relaxed, and when he reaches a sufficient height, enabling the user to move with the VR controllers, to give the feeling of floating in the air or swimming in the water. Thanks to the movement the user could explore a peacful place, and be transported into a world of exoticism, like somewhere in space, high and unusual mountains from the other side of the globe, a zen garden etc.

#### These were the steps for the prototype development:
- Setting up the communication between unreal engine and the device to receive the meditation value inside Unreal Engine
- Development of an algorithm adapting the meditation value provided to overcome its constraints because the value fluctuates too fast to be usable for this usage
- Raising the player when in a meditative state, and lower him/her otherwise
- Add a textual tutorial
- Added movement in the air with VR controllers
#### Remaining tasks from my initial plan:
- Polishing the movement to make them more smooth and more faithful to the hand movements
- Integration of a map to make exploration relaxing and enjoyable
- Integrate localization to support multiple languages for explanatory texts

Moving in VR is a difficult topic as moving with the joystick can cause motion sickness. Moving with the hands can give the feeling of moving a wheel chair or swimming and seem to be less prone to motion sickness, but it needs to feel realistic to avoid it.

[You can see the prototype here](https://youtu.be/DAHYYUaiII8)

On the video I got lucky and was relaxed enough to rise almost non-stop which doesn't happen usually.
Bear in mind that as this was research-oriented around EEG, there isn't any fancy juicyness or gameplay.
