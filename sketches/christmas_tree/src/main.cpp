#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//for LED status
#include <Ticker.h>
Ticker ticker;


// -- global info --
  #include <namepins.h>
    // namepins stores
      // device name global variable - thisDeviceName
      // device path global variable - thisDevicePath
      // subscribe path global variable - subscribe_path (thisDevicePath += "/#")
      // startup pins according to board type
      // the lookup function for associating user defined endpoint names with endpoint_static_id during compile time
  #include <wifilogin.h>
    // located in platformio library folder (usually [home]/.platformio/lib)
    // stores const char ssid and const char password


  String chip_id = String(ESP.getChipId());
  String confirmPath = "";
  String confirmPayload = "";
  String local_ip_str = "";

  String error_path = "";

  String lastSelected = "";

  int redValue = 0;
  int greenValue = 0;
  int blueValue = 0;

  int primaryRedValue = 255;
  int primaryGreenValue = 0;
  int primaryBlueValue = 0;

  int secondaryRedValue = 0;
  int secondaryGreenValue = 255;
  int secondaryBlueValue = 0;


  boolean neoPixelChange = false;

  boolean isAnimating = false;

  boolean timeoutPlay = true;

  static uint32_t MQTTtick = 0;
  static uint32_t MQTTlimit = 300;

  long lastMQTT = 0;

// -- MQTT server setup
  #include <PubSubClient.h>
  IPAddress server(192, 168, 1, 160);

  #define BUFFER_SIZE 100

  WiFiClient espClient;
  PubSubClient client(espClient, server, 1883);
  boolean sendConfirm = false;



// -- NeoPixelBus
  #include <NeoPixelBus.h>
  #include <NeoPixelAnimator.h>

  const uint16_t PixelCount = 150; // this example assumes 4 pixels, making it smaller will cause a failure
  const uint8_t PixelPin = 2;  // make sure to set this to the correct pin, ignored for Esp8266

  #define colorSaturation 256

  // three element pixels, in different order and speeds
  //NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
  //NeoPixelBus<NeoRgbFeature, Neo400KbpsMethod> strip(PixelCount, PixelPin);

  // You can also use one of these for Esp8266,
  // each having their own restrictions
  //
  // These two are the same as above as the DMA method is the default
  // NOTE: These will ignore the PIN and use GPI03 pin
  //NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(PixelCount, PixelPin);
  //NeoPixelBus<NeoRgbFeature, NeoEsp8266Dma400KbpsMethod> strip(PixelCount, PixelPin);

  // Uart method is good for the Esp-01 or other pin restricted modules
  // NOTE: These will ignore the PIN and use GPI02 pin
  //NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> strip(PixelCount, PixelPin);
  
  NeoPixelBus<NeoRgbFeature, NeoEsp8266Uart800KbpsMethod> strip(PixelCount, PixelPin);

  //NeoPixelBus<NeoGrbFeature, NeoEsp8266AsyncUart800KbpsMethod> strip(PixelCount, PixelPin);

  //NeoPixelBus<NeoRgbFeature, NeoEsp8266Uart400KbpsMethod> strip(PixelCount, PixelPin);

  // The bitbang method is really only good if you are not using WiFi features of the ESP
  // It works with all but pin 16
  //NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> strip(PixelCount, PixelPin);
  //NeoPixelBus<NeoRgbFeature, NeoEsp8266BitBang400KbpsMethod> strip(PixelCount, PixelPin);

  // four element pixels, RGBW
  //NeoPixelBus<NeoRgbwFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);


  // solid color stuff

    RgbColor red(colorSaturation, 0, 0);
    RgbColor green(0, colorSaturation, 0);
    RgbColor blue(0, 0, colorSaturation);
    RgbColor white(colorSaturation);
    RgbColor black(0);

    HslColor hslRed(red);
    HslColor hslGreen(green);
    HslColor hslBlue(blue);
    HslColor hslWhite(white);
    HslColor hslBlack(black);

  const uint16_t global_wait = 50;




  boolean solidOverride = true;

  // -- fun fade -- currently sticks, change animation effect state overrides to fade to black
    uint16_t effectState = 0;  // general purpose variable used to store effect state
    const uint8_t AnimationChannels = 1; // we only need one as all the pixels are animated at once
    NeoPixelAnimator FunFadeAnim(AnimationChannels);

    uint16_t FunFadeCount = 0;

    boolean colorStick = false;
    boolean alternateColors = false;
    boolean clearFirst = false; 

    struct FunFadeState
    {
        RgbColor StartingColor;
        RgbColor EndingColor;
        RgbColor SecondaryStartingColor;
        RgbColor SecondaryEndingColor;
    };
    
    // one entry per pixel to match the animation timing manager
    FunFadeState FunFadeAnimationState[AnimationChannels];

    // simple blend function
    void BlendAnimUpdate(const AnimationParam& param)
    {
        // this gets called for each animation on every time step
        // progress will start at 0.0 and end at 1.0
        // we use the blend function on the RgbColor to mix
        // color based on the progress given to us in the animation
        RgbColor updatedColor = RgbColor::LinearBlend(
            FunFadeAnimationState[param.index].StartingColor,
            FunFadeAnimationState[param.index].EndingColor,
            param.progress);

        if (alternateColors) {
          RgbColor updatedSecondaryColor = RgbColor::LinearBlend(
              FunFadeAnimationState[param.index].SecondaryStartingColor,
              FunFadeAnimationState[param.index].SecondaryEndingColor,
              param.progress);

          // apply the color to the strip
          for (uint16_t pixel = 0; pixel < PixelCount; pixel++)
          {
            // if pixel is odd number, set to updatedPrimary, otherwise set updatedSecondary
            if ((pixel % 2) == 0) {
              strip.SetPixelColor(pixel, updatedColor);
            }
            else {
             strip.SetPixelColor(pixel, updatedSecondaryColor); 
            }
          }

        } else {

          // apply the color to the strip
          for (uint16_t pixel = 0; pixel < PixelCount; pixel++)
          {
              strip.SetPixelColor(pixel, updatedColor);
          }

        }


    }

    void FadeInFadeOutRinseRepeat(float luminance)
    {
        if (colorStick) {
          effectState = 0;
        }

        if (effectState == 0)
        {
            // Fade upto a random color
            // we use HslColor object as it allows us to easily pick a hue
            // with the same saturation and luminance so the colors picked
            // will have similiar overall brightness
            uint16_t time = random(800, 2000);

            // alternate colors section
            if (alternateColors) {
              RgbColor target = RgbColor(primaryRedValue, primaryGreenValue, primaryBlueValue);
              RgbColor secondaryTarget = RgbColor(secondaryRedValue, secondaryGreenValue, secondaryBlueValue); 
              time = random(500,1000);

              if ((FunFadeCount % 2) == 0) {

                FunFadeAnimationState[0].StartingColor = target;
                FunFadeAnimationState[0].EndingColor = secondaryTarget;
                FunFadeAnimationState[0].SecondaryStartingColor = secondaryTarget;
                FunFadeAnimationState[0].SecondaryEndingColor = target;


              }
              else {

                FunFadeAnimationState[0].StartingColor = secondaryTarget;
                FunFadeAnimationState[0].EndingColor = target;
                FunFadeAnimationState[0].SecondaryStartingColor = target;
                FunFadeAnimationState[0].SecondaryEndingColor = secondaryTarget;

              }
            }

            // everything else

            else {
              RgbColor target = HslColor(random(360) / 360.0f, 1.0f, luminance);
              FunFadeAnimationState[0].EndingColor = target;
              FunFadeAnimationState[0].StartingColor = strip.GetPixelColor(0);
            }

            


            FunFadeAnim.StartAnimation(0, time, BlendAnimUpdate);
        }
        else if (effectState == 1)
        {
            // fade to black
            uint16_t time = random(600, 700);

            FunFadeAnimationState[0].StartingColor = strip.GetPixelColor(0);
            FunFadeAnimationState[0].EndingColor = RgbColor(0);
            
            FunFadeAnim.StartAnimation(0, time, BlendAnimUpdate);
        }

        // toggle to the next effect state
        effectState = (effectState + 1) % 2;
    }

  // -- fun loop -- new global variables and custom animations

  
    const uint16_t AnimCount = PixelCount / 5 * 2 + 1; // we only need enough animations for the tail and one extra
    const uint16_t PixelFadeDuration = 300; // third of a second
    // one second divide by the number of pixels = loop once a second
    const uint16_t NextPixelMoveDuration = 1000 / PixelCount; // how fast we move through the pixels
    NeoGamma<NeoGammaTableMethod> colorGamma; // for any fade animations, best to correct gamma

    boolean animReverse = false;
    boolean wipeSingle = false;
    boolean keepOrig = false;

    uint16_t FunLoopCount = 0;

    // what is stored for state is specific to the need, in this case, the colors and
    // the pixel to animate;
    // basically what ever you need inside the animation update function
    struct FunLoopState
    {
        RgbColor StartingColor;
        RgbColor EndingColor;
        uint16_t IndexPixel; // which pixel this animation is effecting
    };
    FunLoopState animationState[AnimCount];
    NeoPixelAnimator FunLoopAnim(AnimCount);
    uint16_t frontPixel = 0;  // the front of the loop
    RgbColor frontColor;  // the color at the front of the loop
    uint16_t loopCounter = 0;


    void FadeOutAnimUpdate(const AnimationParam& param)
    {
        // this gets called for each animation on every time step
        // progress will start at 0.0 and end at 1.0
        // we use the blend function on the RgbColor to mix
        // color based on the progress given to us in the animation
        RgbColor updatedColor = RgbColor::LinearBlend(
            animationState[param.index].StartingColor,
            animationState[param.index].EndingColor,
            param.progress);
        // apply the color to the strip
        strip.SetPixelColor(animationState[param.index].IndexPixel, 
        colorGamma.Correct(updatedColor));
    }

    void FunLoopAnimUpdate(const AnimationParam& param)
    {
        // wait for this animation to complete,
        // we are using it as a timer of sorts
        if (param.state == AnimationState_Completed)
        {

            // done, time to restart this position tracking animation/timer
            FunLoopAnim.RestartAnimation(param.index);

            // pick the next pixel inline to start animating


            // -- reverse
            if (animReverse) {
              //frontPixel = (frontPixel - 1) % PixelCount; // increment and wrap
              frontPixel = PixelCount - loopCounter;
              loopCounter = (loopCounter + 1) % (PixelCount + 1);


            }
            // -- forward
            else {
              frontPixel = (frontPixel + 1) % PixelCount; // increment and wrap
            }


            if (frontPixel == 0)
            {
                // we looped, lets pick a new front color
                frontColor = HslColor(random(360) / 360.0f, 1.0f, 0.25f);
                FunLoopCount--;
                if (FunLoopCount == 0) {
                  FunLoopAnim.StopAnimation(0);
                }

            }

            uint16_t indexAnim;
            // do we have an animation available to use to animate the next front pixel?
            // if you see skipping, then either you are going to fast or need to increase
            // the number of animation channels
            if (FunLoopAnim.NextAvailableAnimation(&indexAnim, 1))
            {

              // wipe -- fade back to front randomly chosen color
              if (wipeSingle) {
               animationState[indexAnim].EndingColor = frontColor;
              }

              // pulse -- fade back to the color before the animation -- this doesn't work so I removed it from the menu
              else if (keepOrig) {
                animationState[indexAnim].EndingColor = strip.GetPixelColor(frontPixel);
              }

              // flare -- fade to black
              else {
                animationState[indexAnim].EndingColor = RgbColor(0, 0, 0);
              }


                animationState[indexAnim].StartingColor = frontColor;
                animationState[indexAnim].IndexPixel = frontPixel;

                FunLoopAnim.StartAnimation(indexAnim, PixelFadeDuration, FadeOutAnimUpdate);
            }
        }
    }

  // -- fun random change

    NeoPixelAnimator FunRandomChange(PixelCount);

    uint16_t FunRandomCount = 0;

    struct FunRandomChangeState
    {
      RgbColor StartingColor;
      RgbColor EndingColor;
    };

    FunRandomChangeState FunRandomAnimationState[PixelCount];

    // boolean clearFirst = false; // declared in fun fade section
    boolean allWhite = false;

    // simple blend function
    void FunRandomBlendAnimUpdate(const AnimationParam& param)
    {
        // this gets called for each animation on every time step
        // progress will start at 0.0 and end at 1.0
        // we use the blend function on the RgbColor to mix
        // color based on the progress given to us in the animation
        RgbColor updatedColor = RgbColor::LinearBlend(
            FunRandomAnimationState[param.index].StartingColor,
            FunRandomAnimationState[param.index].EndingColor,
            param.progress);
        // apply the color to the strip
        strip.SetPixelColor(param.index, updatedColor);
    }

    void ClearFirst() {
      // clear all pixels first
        for (uint16_t pixel = 0; pixel < PixelCount; pixel++) {
          strip.SetPixelColor(pixel, RgbColor(0));
        }    
    }


    void PickRandom(float luminance)
    {

        if (clearFirst) {
          ClearFirst();
        }

        // pick random count of pixels to animate
        uint16_t count = random(PixelCount);
        while (count > 0)
        {
            // pick a random pixel
            uint16_t pixel = random(PixelCount);

            // pick random time and random color
            // we use HslColor object as it allows us to easily pick a color
            // with the same saturation and luminance 
            uint16_t time = random(100, 400);

            FunRandomAnimationState[pixel].StartingColor = strip.GetPixelColor(pixel);

            if (allWhite) {
              FunRandomAnimationState[pixel].EndingColor = RgbColor(255);
            }
            else {
              FunRandomAnimationState[pixel].EndingColor = HslColor(random(360) / 360.0f, 1.0f, luminance);
            }

            FunRandomChange.StartAnimation(pixel, time, FunRandomBlendAnimUpdate);

            count--;
        }
    }

  // -- random color
    NeoPixelAnimator RandomColorAnim(PixelCount, NEO_CENTISECONDS);

    uint16_t RandomCount = 0;

    void SetupRandomColor()
    {

        // setup some animations
        for (uint16_t pixel = 0; pixel < PixelCount; pixel++)
        {
            const uint8_t peak = 128;

            // pick a random duration of the animation for this pixel
            // since values are centiseconds, the range is 1 - 4 seconds
            uint16_t time = random(100, 400);

            // each animation starts with the color that was present
            RgbColor originalColor = strip.GetPixelColor(pixel);
            // and ends with a random color
            RgbColor targetColor = RgbColor(random(peak), random(peak), random(peak));
            // with the random ease function
            AnimEaseFunction easing;

            switch (random(3))
            {
            case 0:
                easing = NeoEase::CubicIn;
                break;
            case 1:
                easing = NeoEase::CubicOut;
                break;
            case 2:
                easing = NeoEase::QuadraticInOut;
                break;
            }


          // we must supply a function that will define the animation, in this example
          // we are using "lambda expression" to define the function inline, which gives
          // us an easy way to "capture" the originalColor and targetColor for the call back.
          //
          // this function will get called back when ever the animation needs to change
          // the state of the pixel, it will provide a animation progress value
          // from 0.0 (start of animation) to 1.0 (end of animation)
          //
          // we use this progress value to define how we want to animate in this case
          // we call RgbColor::LinearBlend which will return a color blended between
          // the values given, by the amount passed, hich is also a float value from 0.0-1.0.
          // then we set the color.
          //
          // There is no need for the MyAnimationState struct as the compiler takes care
          // of those details for us
          
          
          AnimUpdateCallback animUpdate = [=](const AnimationParam& param)
          {
              // progress will start at 0.0 and end at 1.0
              // we convert to the curve we want
              float progress = easing(param.progress);

              // use the curve value to apply to the animation
              RgbColor updatedColor = RgbColor::LinearBlend(originalColor, targetColor, progress);
              strip.SetPixelColor(pixel, updatedColor);
          };


          // now use the animation properties we just calculated and start the animation
          // which will continue to run and call the update function until it completes
          RandomColorAnim.StartAnimation(pixel, 500, animUpdate);

        }
    }

  // -- rainbow, adapted from random color

    NeoPixelAnimator RainbowAnim(PixelCount, NEO_CENTISECONDS);

    void SetupRainbow()
    {

        // pick a random color for the starting pixel
         uint16_t startingFrontHueValue = random(1, 255);

        // with the random ease function
        AnimEaseFunction easing;

        switch (random(3))
        {
        case 0:
            easing = NeoEase::CubicIn;
            break;
        case 1:
            easing = NeoEase::CubicOut;
            break;
        case 2:
            easing = NeoEase::QuadraticInOut;
            break;
        }


        // setup some animations
        for (uint16_t pixel = 0; pixel < PixelCount; pixel++)
        {

          // each animation starts with the starting color, plus the pixel number, rolling over after 255
          uint16_t pixelOrigHueValue = startingFrontHueValue + pixel;

          if (pixelOrigHueValue > 255) {
            pixelOrigHueValue = pixelOrigHueValue - 255;
          }
          float pixelOrigHueValueFloat = (float) pixelOrigHueValue;

          HslColor pixelOriginalHue ((pixelOrigHueValueFloat / 360.0f), 1.0f, 0.5f);

          HslColor pixelFinalHue ((pixelOrigHueValueFloat - 1.0f) / 360.0f, 1.0f, 0.5f);



          AnimUpdateCallback rainbowUpdate = [=](const AnimationParam& param)
          {
              
              // progress will start at 0.0 and end at 1.0
              // we convert to the curve we want
              float progress = easing(param.progress);

              // use the curve value to apply to the animation
              HslColor updatedColor = HslColor::LinearBlend<NeoHueBlendLongestDistance>(pixelOriginalHue, pixelFinalHue, progress);
              strip.SetPixelColor(pixel, updatedColor);
              
          };

          // now use the animation properties we just calculated and start the animation
          // which will continue to run and call the update function until it completes
          RainbowAnim.StartAnimation(pixel, 1000, rainbowUpdate);

        }

    }
    



  // -- assorted other functions

    void SetRandomSeed()
    {
        uint32_t seed;

        // random works best with a seed that can use 31 bits
        // analogRead on a unconnected pin tends toward less than four bits
        seed = analogRead(0);
        delay(1);

        for (int shifts = 3; shifts < 31; shifts += 3)
        {
            seed ^= analogRead(0) << shifts;
            delay(1);
        }

        // Serial.println(seed);
        randomSeed(seed);
    }

    void StopAllAnimations() {
      FunRandomCount = 0;
      FunFadeCount = 0;
      FunLoopCount = 0;
      RandomCount = 0;
      FunLoopAnim.StopAnimation(0);
    }

// -- NeoPixelAnimationFunctions

  void FunRandom() {                
    clearFirst = false;
    allWhite = false;
    FunRandomCount = 10;
    PickRandom(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright
  }


  void RandomSparkle() {                
    clearFirst = true;
    allWhite = false;
    FunRandomCount = 10;
    PickRandom(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright
  }
  
  void WhiteSparkle() {                
    clearFirst = true;
    allWhite = true;
    FunRandomCount = 10;
    PickRandom(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright
  }

  void FadeStripInOut() {
    effectState = 0;
    FunFadeCount = 19; 
    colorStick = false;
    alternateColors = false;
    FadeInFadeOutRinseRepeat(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright
  }
  void FadeStripIn() {
    effectState = 0;
    FunFadeCount = 20;
    colorStick = true;
    alternateColors = false;
    FadeInFadeOutRinseRepeat(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright
  }
  void ClearAndFadeStripIn() {
    effectState = 0;
    FunFadeCount = 20; 
    colorStick = true;
    ClearFirst();
    alternateColors = false;
    FadeInFadeOutRinseRepeat(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright
  }
  void AlternateFadeIn() {
    effectState = 0;
    FunFadeCount = 20; 
    colorStick = true;
    alternateColors = true;
    FadeInFadeOutRinseRepeat(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright
  }
  void AlternateFadeInOut() {
    effectState = 0;
    FunFadeCount = 20; 
    colorStick = false;
    alternateColors = true;
    FadeInFadeOutRinseRepeat(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright              
  }

  void Flare() {
    animReverse = false;
    wipeSingle = false;
    keepOrig = false;
    FunLoopCount = 10;
    FunLoopAnim.StartAnimation(0, NextPixelMoveDuration, FunLoopAnimUpdate);
  }
  void FlareReverse() {
    animReverse = true;
    wipeSingle = false;
    keepOrig = false;
    FunLoopCount = 10;
    FunLoopAnim.StartAnimation(0, NextPixelMoveDuration, FunLoopAnimUpdate);
  }
  void ColorWipe() {
    animReverse = false;
    wipeSingle = true;
    keepOrig = false;
    FunLoopCount = 10;
    FunLoopAnim.StartAnimation(0, NextPixelMoveDuration, FunLoopAnimUpdate);
  }
  void ColorWipeReverse() {
    animReverse = true;
    wipeSingle = true;
    keepOrig = false;
    FunLoopCount = 10;
    FunLoopAnim.StartAnimation(0, NextPixelMoveDuration, FunLoopAnimUpdate);                
  }

  void chooseAnimation() {

    if (lastSelected == "Fun Random\"}") {
      FunRandom();
    }
    else if (lastSelected == "Random Sparkle\"}") {
      RandomSparkle();
    }
    else if (lastSelected == "White Sparkle\"}") {
      WhiteSparkle();
    }
    else if (lastSelected == "Fade Strip In Out\"}") {
      FadeStripInOut();
    }
    else if (lastSelected == "Fade Strip In\"}") {
      FadeStripIn();
    }
    else if (lastSelected == "Clear and Fade Strip In\"}") {
      ClearAndFadeStripIn();
    }
    else if (lastSelected == "Alternate Fade In\"}") {
      AlternateFadeIn();
    }
    else if (lastSelected == "Alternate Fade In Out\"}") {
      AlternateFadeInOut();
    }
    else if (lastSelected == "Flare\"}") {
      Flare();
    }
    else if (lastSelected == "Flare Reverse\"}") {
      FlareReverse();
    }
    else if (lastSelected == "Color Wipe\"}") {
      ColorWipe();
    }
    else if (lastSelected == "Color Wipe Reverse\"}") {
      ColorWipeReverse();
    }
    else if (lastSelected == "Random Color\"}") {
      RandomCount = 10;
      SetupRandomColor();
    }
    else if (lastSelected == "Rainbow\"}") {
      SetupRainbow();
    }

  }






// -- TICK FUNTION
  void tick_fnc()
  {
    //toggle state
    int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
    digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
  }

// -- MQTT functions

  String getValue(String data, char separator, int index)
  {
   yield();
   int found = 0;
    int strIndex[] = {0, -1  };
    int maxIndex = data.length()-1;
    for(int i=0; i<=maxIndex && found<=index; i++){
      if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
      }
   }
    return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
  }

  // --------------------------------------------------------
  // HIGH LEVEL OVERVIEW OF ESP AND PERSISTENCE COMMUNICATION
  // --------------------------------------------------------
  // ask persistence/control/device_name/chipID "request states" --  do you have any states with my device_name or chipID
    // if not, receive reply from deviceInfo/control/device_name "no states"
      // send deviceInfo/confirm/device_name {default object} -- send the device info object with default endpoints included
        // NOTE: This step probably isn't necessary. persistence should always have

    // if yes, receive each endpoint from [device path]/control/[device_name]/[endpoint_key] (camelized card title)
      // receive each endpoint one by one and it's corresponding value

        // send endpoint_key to function stored in namepins.h at compile time
        // function returns static_endpoint_id associated with that endpoint
        // sketch acts on that value as it normally would, using the static_endpoint_id to know for sure what it should do (turn output pin on/off, adjust RGB light, etc)
        // sketch confirms the value by sending it back on /[path]/[confirm]/[device_name]/[endpoint_key]



  String topic;
  String payload;
  void callback(const MQTT::Publish& pub) {
    yield();
    if (millis() - MQTTtick > MQTTlimit) {
      MQTTtick = millis();


      int commandLoc;
      String command = "";
      String deviceName = "";
      String endPoint = "";

      topic = pub.topic();
      //payload = pub.payload_string(); // -- trying a hack to avoid processing a large payload when persistence sends device to dashboard

      // -- topic parser
          // syntax:
          // global: / global / path / command / function
          // device setup: / deviceInfo / command / name
          // normal: path / command / name / endpoint

      // check item 1 in getValue
      String firstItem = getValue(topic, '/', 1);
      if (firstItem == "global") {
        // -- do nothing until I change to $ prefix before command types
      }
      else if (firstItem == "deviceInfo") {
        // get name and command
        deviceName = getValue(topic, '/', 3);
        command = getValue(topic, '/', 2);
        if ((deviceName == thisDeviceName) && (command == "control")) {

          payload = pub.payload_string();
          
          if (payload == "no states") {
            // -- do something to send the default states, but now that this is managed by persistence so this shouldn't be necessary
            // -- maybe it just resets all the states to 0 or whatever was originally programmed into the sketch
            //sendJSON = true;
          }
          else if (payload == "blink on") {
            Serial.end();
            pinMode(BUILTIN_LED, OUTPUT);
            ticker.attach(0.6, tick_fnc);
          }
          else if (payload == "blink off") {
            ticker.detach();
            digitalWrite(BUILTIN_LED, HIGH);
            pinMode(BUILTIN_LED, INPUT);
            Serial.begin(115200);
          }
          else {
            // -- persistence will no longer send the default object, if it's an arduino based esp chip, it will just send the control messages to /[device_path]/control/[device_name]/[endpoint_key]
          }



        }

      }
      else {

        payload = pub.payload_string();

        int i;
        int maxitems;

        // count number of items
        for (i=1; i<topic.length(); i++) {
          String chunk = getValue(topic, '/', i);
          if (chunk == NULL) {
            break;
          }
        }

        // get topic variables
        maxitems = i;

        for (i=1; i<maxitems; i++) {
          String chunk = getValue(topic, '/', i);
          if (chunk == "control") {
            commandLoc = i;
            command = chunk;
            deviceName = getValue(topic, '/', i + 1);
            endPoint = getValue(topic, '/', i + 2);
            break;
          }
        }


        if (deviceName == thisDeviceName) {

          //Serial.println("device and endpoint incoming...");
          //Serial.println(deviceName);
          //Serial.println(endPoint);

          // send endpoint_key to function stored in namepins.h at compile time
          // function returns static_endpoint_id associated with that endpoint

          String lookup_val = lookup(endPoint);
          //Serial.println("looking value incoming...");
          //Serial.println(lookup_val);


          // sketch acts on that value as it normally would, using the static_endpoint_id to know for sure what it should do (turn output pin on/off, adjust RGB light, etc)
          if (lookup_val == "RGB") {

            lastMQTT = millis();
            StopAllAnimations();
            solidOverride = true;

            // deserialize payload, get valueKey
            // or just look for value or red,green,blue
            String findKey = getValue(payload, '"', 1);
            String findValue = getValue(payload, ':', 1);
            findValue.remove(findValue.length() - 1);

            if (findKey == "red") {
              redValue = findValue.toInt();
            }
            else if (findKey == "green") {
              greenValue = findValue.toInt();
            }
            else if (findKey == "blue") {
              blueValue = findValue.toInt();
            }

          }
          else if (lookup_val == "primaryColor") {

            // deserialize payload, get valueKey
            // or just look for value or red,green,blue
            String findKey = getValue(payload, '"', 1);
            String findValue = getValue(payload, ':', 1);
            findValue.remove(findValue.length() - 1);

            if (findKey == "red") {
              primaryRedValue = findValue.toInt();
            }
            else if (findKey == "green") {
              primaryGreenValue = findValue.toInt();
            }
            else if (findKey == "blue") {
              primaryBlueValue = findValue.toInt();
            }

          }
          else if (lookup_val == "secondaryColor") {

            // deserialize payload, get valueKey
            // or just look for value or red,green,blue
            String findKey = getValue(payload, '"', 1);
            String findValue = getValue(payload, ':', 1);
            findValue.remove(findValue.length() - 1);

            if (findKey == "red") {
              secondaryRedValue = findValue.toInt();
            }
            else if (findKey == "green") {
              secondaryGreenValue = findValue.toInt();
            }
            else if (findKey == "blue") {
              secondaryBlueValue = findValue.toInt();
            }

          }
                    
          else if (lookup_val == "animationMenu") {

            lastMQTT = millis();
            StopAllAnimations();

            solidOverride = false;
            
            lastSelected = payload.substring(10);


            chooseAnimation();
          }



          // sketch confirms the value by sending it back on /[path]/[confirm]/[device_name]/[endpoint_key]

          confirmPath = "";
          confirmPath = thisDevicePath;
          confirmPath += "/confirm/";
          confirmPath += thisDeviceName;
          confirmPath += "/";
          confirmPath += endPoint;
          confirmPayload = payload;

          //sendConfirm = true;
          client.publish(MQTT::Publish(confirmPath, confirmPayload).set_qos(2));

      }
    }
  }




  }


void setup() {

  Serial.begin(115200);
  while (!Serial); // wait for serial attach
  Serial.println("Booting");
  Serial.println(ESP.getResetInfo());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }


  // -- OTA
    // OTA options
    // Port defaults to 8266
    // ArduinoOTA.setPort(8266);
    // Hostname defaults to esp8266-[ChipID]
    // ArduinoOTA.setHostname("myesp8266");
    // No authentication by default
    // ArduinoOTA.setPassword((const char *)"123");

    ArduinoOTA.onStart([]() {
      Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  local_ip_str = WiFi.localIP().toString();
  Serial.println(local_ip_str);

  // neopixel bus
  SetRandomSeed();
  strip.Begin();
  strip.Show();

}



void loop() {
  ArduinoOTA.handle();

  // -- MQTT connect
    if (!client.connected()) {
      error_path += thisDevicePath;
      error_path += "/";
      error_path += "errors/";
      error_path += thisDeviceName;
      if (client.connect(MQTT::Connect(thisDeviceName).set_will(error_path, "disconnected"))) {
        Serial.println("MQTT connected");
        client.set_callback(callback);
        client.subscribe(MQTT::Subscribe()
          .add_topic("/deviceInfo/#")
          .add_topic("/global/#")
          .add_topic(subscribe_path)
                 );
        // notify persistence of device IP
          String persistence_ip_path = "/persistence/control/";
          persistence_ip_path += thisDeviceName;
          persistence_ip_path += "/ip";
          client.publish(MQTT::Publish(persistence_ip_path, local_ip_str).set_qos(2));

        // ask persistence/control/device_name/chipID "request states" --  do you have any states with my device_name or chipID
          String persistence_path = "/persistence/control/";
          persistence_path += thisDeviceName;
          persistence_path += "/";
          persistence_path += chip_id;
          client.publish(MQTT::Publish(persistence_path, "request states").set_qos(2));
          Serial.println("request states sent");
      }
    }

    if (client.connected()) {
      client.loop();
    }


  isAnimating = false;


  // -- NeoPixel continuous update
    if (solidOverride) {
      for(int i=0; i<PixelCount; i++) {
        strip.SetPixelColor(i, RgbColor (redValue, greenValue, blueValue));
      }
    strip.Show();
    }




  if (FunRandomChange.IsAnimating()) {
      // the normal loop just needs these two to run the active animations
      FunRandomChange.UpdateAnimations();
      strip.Show();
      isAnimating = true;
  }
  else if (FunRandomCount > 0) {
    PickRandom(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright
    FunRandomCount--;
    isAnimating = true;
  }




  if (FunFadeAnim.IsAnimating()) {
      // the normal loop just needs these two to run the active animations
      FunFadeAnim.UpdateAnimations();
      strip.Show();
      isAnimating = true;
  }
  else if (FunFadeCount > 0 ) {
    FunFadeCount--;
    FadeInFadeOutRinseRepeat(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright
    isAnimating = true;
  }




  if (FunLoopAnim.IsAnimating()) {
      // the normal loop just needs these two to run the active animations
      FunLoopAnim.UpdateAnimations();
      strip.Show();
      isAnimating = true;
  }




  if (RandomColorAnim.IsAnimating()) {
      // the normal loop just needs these two to run the active animations
      RandomColorAnim.UpdateAnimations();
      strip.Show();
      isAnimating = true;
  }
  else if (RandomCount > 0 ) {
    SetupRandomColor();
    RandomCount--;
    isAnimating = true;
  }




  if (RainbowAnim.IsAnimating()) {
      // the normal loop just needs these two to run the active animations
      RainbowAnim.UpdateAnimations();
      strip.Show();
      isAnimating = true;
  }


  if (isAnimating == false) {

    if (millis() - lastMQTT > 1800000 && (timeoutPlay)) { // 30 minute timer before animations start 

      solidOverride = false;
      int animationRandom = random(0,12);
    
      switch (animationRandom) {
        case 0:
          FunRandom();
          break;
        case 1:
          RandomSparkle();
          break;
        case 2:
          WhiteSparkle();
          break;
        case 3:
          FadeStripInOut();
          break;
        case 4:
          FadeStripIn();
          break;
        case 5:
          AlternateFadeIn();
          break;
        case 6:
          AlternateFadeInOut();
          break;
        case 7:
          Flare();
          break;
        case 8:
          FlareReverse();
          break;
        case 9:
          ColorWipe();
          break;
        case 10:
          ColorWipeReverse();
          break;
        case 11:
          RandomCount = 10;
          SetupRandomColor();
          break;
        case 12:
          SetupRainbow();
          break;
      }      

    }
    else {
      if (!solidOverride) {
        chooseAnimation();
      }
    }
    
  }

  yield();
}

