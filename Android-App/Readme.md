### Android App (.apk)

### To Do
+ Implement a heart-beat function that does not prevent the phone going to sleep, and then getting messages that this application is prevent that sleep mode thus resulting in reduced battery (quite ironic, really) (functional) 
+ Tidy up the front screen (cosmetic)
+ Tidy up the About screen (cosmetic)
+ Investigate getting the app into the Play Store
+ Anything else I find along the way

The current .apk application (in this folder) is fully functional with the caveat that Android ocassionally complains that a Power-Intensive App is preventing phone sleep mode. Not strictly true. This app consumes very little power (only when sending BlueTooth messages back to the Arduino) but does prevent the phone sleeping. Workaround: when not actively charging, quit the app.
