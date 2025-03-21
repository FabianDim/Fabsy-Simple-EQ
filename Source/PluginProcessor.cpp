/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
//#include <include_juce_audio_devices.cpp>

//==============================================================================
FabsysSimpleEQAudioProcessor::FabsysSimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

FabsysSimpleEQAudioProcessor::~FabsysSimpleEQAudioProcessor()
{
}

//==============================================================================
const juce::String FabsysSimpleEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FabsysSimpleEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FabsysSimpleEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FabsysSimpleEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FabsysSimpleEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FabsysSimpleEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FabsysSimpleEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FabsysSimpleEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String FabsysSimpleEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void FabsysSimpleEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void FabsysSimpleEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    /*applying peak filter settings (based on previously obtained chainSettings to both the left and right audio channels of an equalizer*/

    auto chainSettings = getChainSettings(apvts);//calls getChainSettings function retrieving the EQ settings from the AudioProcessorValueTreeState 

    //This line creates filter coefficients 
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));


    //Applying the filter coefficients to the left an right audio channels.
    //get<ChainPositions::Peak>() Retrieves the peak filter component from each processing chain.
    //.coefficients accesses the coefficients (paramters) of the peak filter in each chain
    // *peakCoefficients dereferences the perviously generated filter coefficitents and assigns them to the peak filters coefficients in both the left and rigt channesl
    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;


    //creates a coefficient for every 2 order
    //if our choice is at 0 we want to get back one coefficient object therefore we must supply an order of two.

    /*This method returns an array of IIR::Coefficients, made to be used in cascaded IIRFilters,
    providing a minimum phase high-pass filter without any ripple in the pass band and in the stop band.*/
    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, sampleRate, 2 * (static_cast<int>(chainSettings.lowCutSlope) + 1));//order is third param
    auto& leftLowCut = leftChain.get < ChainPositions::LowCut > ();

    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);
}

void FabsysSimpleEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FabsysSimpleEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void FabsysSimpleEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto chainSettings = getChainSettings(apvts);//calls getChainSettings function retrieving the EQ settings from the AudioProcessorValueTreeState 

    //This line creates filter coefficients 
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));


    //Applying the filter coefficients to the left an right audio channels.
    //get<ChainPositions::Peak>() Retrieves the peak filter component from each processing chain.
    //.coefficients accesses the coefficients (paramters) of the peak filter in each chain
    // *peakCoefficients dereferences the perviously generated filter coefficitents and assigns them to the peak filters coefficients in both the left and rigt channesl
    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

    juce::dsp::AudioBlock<float> block(buffer);

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    
    leftChain.process(leftContext);
    rightChain.process(rightContext);
}

//==============================================================================
bool FabsysSimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FabsysSimpleEQAudioProcessor::createEditor()
{
    //return new FabsysSimpleEQAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void FabsysSimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void FabsysSimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings; //Retreives EQ setting (freq, gain etc) from audioProcessorvalue tree state and returns them encapsulated in a chainsettings object.

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load(); //returns the value in units we care about 
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());

    return settings;
}

juce::AudioProcessorValueTreeState::ParameterLayout FabsysSimpleEQAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique <juce::AudioParameterFloat>("LowCut Freq", "LowCut Freq", //adding to the layout the audio parameters individually with maximum and minimum values
        juce::NormalisableRange<float>(20.f, 20000.0f, 1.0f, 1.0f, 0.25f),20.f)); //this is the frequency parameters 

    layout.add(std::make_unique <juce::AudioParameterFloat>("HighCut Freq", "HighCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.0f, 1.0f, 0.25f), 20000.0f));

    layout.add(std::make_unique <juce::AudioParameterFloat>("Peak Freq", "Peak Freq",
        juce::NormalisableRange<float>(20.f, 20000.0f, 1.0f, 0.25f), 750.0f));
    
    layout.add(std::make_unique <juce::AudioParameterFloat>("Peak Gain", "Peak Gain",//this is the gain parameters.
        juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f), 0.0f));
    
    layout.add(std::make_unique <juce::AudioParameterFloat>("Peak Quality", "Peak Quality",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.5f, 1.0f), 1.0f));

    juce::StringArray stringArray;

    for (int i = 0; i < 4; i++) { //for each option out of 4 decibel options we are adding multiples of 12 to the string array as an option for the cuttoff of the eq.
        juce::String str; 
        str << (12 + i * 12);
        str << "db/Oct";
        stringArray.add(str);
    }


    layout.add(std::make_unique <juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
    layout.add(std::make_unique <juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FabsysSimpleEQAudioProcessor();
}


