
// libraries
#include <SPI.h>
#include <SdFat.h>
#include <SFEMP3Shield.h>

#define DEBUG 0

#define REPEAT_DIR 0

// define Equalizer parameters
/*
  #define MONOMODE             1
  #define TREBLE_FREQUENCY  2000 // Treble cutoff frequency limit in Hertz (1000Hz to 15000Hz).
  #define TREBLE_AMPLITUDE     4 // Treble amplitude in dB from -8 to 7.
  #define BASS_FREQUENCY     110 // Bass Boost frequency cutoff limit in Hertz (20Hz to 150Hz).
  #define BASS_AMPLITUDE       9 // Bass Boost amplitude in dB (0dB to 15dB).

*/
  #define MONOMODE             0
  #define TREBLE_FREQUENCY  2000 // Treble cutoff frequency limit in Hertz (1000Hz to 15000Hz).
  #define TREBLE_AMPLITUDE     4 // Treble amplitude in dB from -8 to 7.
  #define BASS_FREQUENCY     100 // Bass Boost frequency cutoff limit in Hertz (20Hz to 150Hz).
  #define BASS_AMPLITUDE       5 // Bass Boost amplitude in dB (0dB to 15dB).


// Define Volume Pin
#define VOLUME_PIN    A0
#define KEY_PIN       A1

#define KEY_NEXT      100
#define KEY_PREVIOUS  101

byte oldKeyPressed = 0;

// principal object for handling all SdCard functions.
SdFat sd;
SdFile file;

// principal object for handling all the attributes, members and functions for the library.
SFEMP3Shield MP3player;

// the current volume level, set to min at start
byte volumeState = 254;

byte numberOfFiles[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

byte currentFile = 1;
byte currentDir = 1;
byte offsetDir = 0;
char currentFileName[9] = "001.mp3";
char currentDirName[4] = "/00";
  
char currentTrackFileName[] = "current.txt";
uint8_t result; //result code from some function as to be tested at later time.
  
void setup() {
  // initialize serial communication at 115200 bits per second
  Serial.begin(115200);

  // initialise SD card access
  if(!sd.begin(SD_SEL, SPI_FULL_SPEED)) sd.initErrorHalt();
  if (!sd.chdir("/")) sd.errorHalt("sd.chdir");

  //Initialize the MP3 Player Shield
  result = MP3player.begin();
  //check result, see readme for error codes.
  if(result != 0) {
    Serial.print(F("Error code: "));
    Serial.print(result);
    Serial.println(F(" when trying to start MP3 player"));
    if( result == 6 ) {
      Serial.println(F("Warning: patch file not found, skipping.")); // can be removed for space, if needed.
    }
  }
  
  checkVolume();

  // Configure Equalizer
  MP3player.setTrebleFrequency(TREBLE_FREQUENCY); 
  MP3player.setTrebleAmplitude(TREBLE_AMPLITUDE); 
  MP3player.setBassFrequency(BASS_FREQUENCY);   
  MP3player.setBassAmplitude(BASS_AMPLITUDE);   

  // Set MONO mode
  MP3player.setMonoMode(MONOMODE);
  
  // read last file played if exists
  if (readCurrentTrack() != 0) Serial.println(F("Current Track file could not be read."));

  // check for activation of extended directory set (hold NEXT button during startup
  if (myGetKey() == KEY_NEXT)
  {
    if (offsetDir > 0)
    {
      offsetDir = 0;
    }
    else
    {
      offsetDir = 9;
    }
    MP3player.SendSingleMIDInote();
  }
  
  // read the number of tracks in each folder
  for (byte i = 1; i <= 9; i++)
  {
    sprintf(currentDirName, "/%02d", i + offsetDir);
    if (sd.chdir(currentDirName))
    {
      while (file.openNext(sd.vwd(), O_READ)) {
        
        #if DEBUG
          file.printName();
          Serial.println();
        #endif
         
        numberOfFiles[i]++;
        file.close();
      }
    }
    #if DEBUG
      Serial.print(F("count files: Dir: "));
      Serial.print(currentDirName);
      Serial.print(F(" # of files: "));
      Serial.println(numberOfFiles[i]);
    #endif
  }

  MP3player.SendSingleMIDInote();
  playCurrent();
  Serial.println(F("Init done."));
}

void loop() {
  // play next song if player stopped
  if (!MP3player.isPlaying())
  {
      playNext();
  }

  // check the volume and set it
  checkVolume();

  // check if a button is pressed and perform some action
  checkButtons();

  delay(50); // delay in between reads for stability
}

void playCurrent()
{
  if (numberOfFiles[currentDir] >= currentFile)
  {
    sprintf(currentFileName, "%03d.mp3", currentFile);
    sprintf(currentDirName, "%02d", currentDir + offsetDir);

    MP3player.stopTrack();
    if (saveCurrentTrack() != 0 ) Serial.println(F("void playCurrent(): Current Track not saved."));
    if (!sd.chdir("/")) sd.errorHalt("sd.chdir /");
    if (!sd.chdir(currentDirName)) sd.errorHalt("sd.chdir currentDirname");
    result = MP3player.playMP3(currentFileName);    
    if(result != 0) {
      Serial.print(F("Error code: "));
      Serial.print(result);
      Serial.println(F(" when trying to play "));
      Serial.print(F("DirName: "));
      Serial.println(currentDirName);
      Serial.print(F("FileName: "));
      Serial.println(currentFileName);
    }
    #if DEBUG
      Serial.print(F("Playing: "));
      Serial.print(currentDirName);
      Serial.print(F("/"));
      Serial.println(currentFileName);
    #endif
  }
}

void playNext()
{
  currentFile++;
  if (currentFile > numberOfFiles[currentDir])
  {
    if (REPEAT_DIR > 0) {
      currentFile = 1;
    } else {
      currentFile = 1;
      currentDir++;
      if (currentDir > 9) currentDir = 1;
    }
  }
  playCurrent();
}

void playPrevious()
{
  if (currentFile > 1)
  {
    currentFile--;
  }
  else
  {
    if (REPEAT_DIR) {
      currentFile = 1;
    } else {
      if (currentDir == 0)
      {
        currentDir = 9;
      }
      else
      {
        currentDir--;  
      }
      currentFile = numberOfFiles[currentDir];
    }
  }
  playCurrent();
}

void checkButtons()
{
  // evaluate keypress
  char keyPressed = myGetKey();
  if (keyPressed != oldKeyPressed)
  {
    #if DEBUG
      if(keyPressed > 0) {
        Serial.print(F("Key pressed: "));
        Serial.println(keyPressed);
      }
    #endif

    oldKeyPressed = keyPressed;
    
    if (keyPressed >= 1 && keyPressed <= 9)
    {
      currentFile = 1;
      currentDir = keyPressed;
      playCurrent();
    } 
    else if (keyPressed == KEY_NEXT)
    {
      playNext();
    }
    else if (keyPressed == KEY_PREVIOUS)
    {
      playPrevious();
    }
  }
}


byte myGetKey()
{
  int Reading = analogRead(KEY_PIN);
  Reading = Reading/2 + analogRead(KEY_PIN)/2;
  
  if(Reading > 700) return 1;
  if(Reading > 509) return 2;
  if(Reading > 340) return 3;
  if(Reading > 176) return 4;
  if(Reading > 149) return 5;
  if(Reading > 129) return 6;
  if(Reading > 94) return 7;
  if(Reading > 85) return 8;
  if(Reading > 78) return 9;
  if(Reading > 63) return KEY_PREVIOUS;
  if(Reading > 59) return KEY_NEXT;
  return 0;
}

// FUNKTION get value of the volume poti
void checkVolume() {
  // read the state of the volume potentiometer
  int vol = analogRead(VOLUME_PIN) ; // 0-1023

  // set the range of the volume from max=0 to min=254
  // (limit max volume to 20 and min to 70) 
  byte state = map(vol, 0, 1023, 20, 80);
    
  // recognize state (volume) changes in steps of two
  if (state < volumeState - 1 || state > volumeState + 1)
  {
      // remember the new volume state
      volumeState = state;
  
      // set volume max=0, min=254
      MP3player.setVolume(volumeState);
  }

  #if DEBUG
    Serial.print(F("void setVolume(): Poti Value: "));
    Serial.print(vol);
    Serial.print(F(" Volume Value: "));
    Serial.println(state);
  #endif
}

byte saveCurrentTrack()
{
    if (!sd.chdir("/")) sd.errorHalt("sd.chdir");
    if (sd.exists(currentTrackFileName)) sd.remove(currentTrackFileName);
 
    File myfile = sd.open(currentTrackFileName, FILE_WRITE);
    if (myfile)
    {
        myfile.println(currentDir);
        myfile.println(currentFile);
        myfile.println(offsetDir);
        #if DEBUG
          Serial.print(F("saveCurrentTrack(): currentDir: "));
          Serial.print(currentDir);
          Serial.print(F(" currentFile: "));
          Serial.print(currentFile);
          Serial.print(F(" DIR offset: "));
          Serial.println(offsetDir);
        #endif
    } else {
      return 1;
    }
    myfile.close();
    return 0;
}

byte readCurrentTrack()
{
    if (!sd.chdir("/")) sd.errorHalt("sd.chdir /");
    // read remembered track
    if (sd.exists(currentTrackFileName))
    {
        File myfile = sd.open(currentTrackFileName, FILE_READ);
        if (myfile)
        {
            currentDir = myfile.readStringUntil('\n').toInt();
            currentFile = myfile.readStringUntil('\n').toInt();
            offsetDir = myfile.readStringUntil('\n').toInt();
            if ((currentDir <= 0) || (currentDir > 9)) currentDir = 1;
            if ((currentFile <= 0) || (currentFile > 128)) currentFile = 1;
            if ((offsetDir != 0) || (offsetDir != 9)) offsetDir = 0;
        }
        myfile.close();
    } else {
      currentDir = 1;
      currentFile = 1;
      offsetDir = 0;
      return 1;
    }
    return 0;
}

