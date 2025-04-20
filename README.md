#A 3-Band Audio Equaliser VST Plug-In (C++ / JUCE)

I developed a 3-band audio equaliser plug-in using C++ and the open-source JUCE framework. The plug-in features low-cut, peak, and high-cut filters, giving users flexible control over frequency shaping in real time. It was built to run as both a VST and a standalone application, making it easy to test and integrate into digital audio workstations (DAWs).

One of the biggest challenges was understanding JUCE’s DSP module and the signal chain's structure. Managing multiple filter states for both stereo channels and dynamically updating coefficients based on real-time user input was tricky at first. Debugging issues like incorrect filter slope behavior or broken bypass logic required careful tracing through the signal path. I often found myself diving deep into JUCE’s source code just to fully understand how things were working under the hood.

Another hurdle was configuring the standalone version of the plugin properly—especially making sure the audio device settings and parameter states were preserved across sessions. At one point, undoing a Git commit accidentally broke the settings interface, which forced me to retrace and debug my setup using JUCE’s built-in StandalonePluginHolder logic.

To work through these challenges, I broke the problem down into smaller parts, used AudioProcessorValueTreeState for reliable parameter management, and gradually built out a modular system for applying and updating filter settings. JUCE’s real-time debugging tools and community forums were also huge in helping me get unstuck.

The project gave me hands-on experience with digital signal processing, plugin architecture, and real-world C++ debugging, and it strengthened my confidence working with complex frameworks in a creative, performance-critical context.
