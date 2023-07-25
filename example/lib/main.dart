import 'package:flutter/material.dart';
import 'package:flutter_sarec/flutter_sarec.dart' as flutter_sarec;

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  int client = 0;
  @override
  void initState() {
    client = flutter_sarec.createSarecClient();
    print("client:$client");
    super.initState();
  }

  bool recordingStarted = false;
  bool paused = false;

  @override
  void dispose() {
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        body: Column(
          children: [
            ElevatedButton(
              onPressed: () {
                if (recordingStarted) {
                  setState(() {
                    flutter_sarec.stopRecording(client);
                    recordingStarted = false;
                  });
                } else {
                  setState(() {
                    flutter_sarec.startRecording(client, "./sample.raw");
                    recordingStarted = true;
                  });
                }
              },
              child:
                  Text(recordingStarted ? "Stop Recording" : "Start Recording"),
            ),
            ElevatedButton(
              onPressed: () {
                print("On Pause clicked");
                if (!recordingStarted) {
                  setState(() {
                    paused = false;
                  });
                  return;
                }

                print("Here");

                if (!paused) {
                  setState(() {
                    flutter_sarec.pauseRecording(client);
                    paused = true;
                  });
                } else {
                  setState(() {
                    flutter_sarec.resumeRecording(client);
                    paused = false;
                  });
                }
              },
              child: Text(!paused ? "Pause Recording" : "Resume Recording"),
            ),
          ],
        ),
      ),
    );
  }
}
