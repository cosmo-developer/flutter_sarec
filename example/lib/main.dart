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

    flutter_sarec.startRecording(client, "./roothless.raw");
    super.initState();
  }

  @override
  void dispose() {
    // TODO: implement dispose
    flutter_sarec.destroySarecClient(client);
    print("Disposing");
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return const MaterialApp(
      home: Scaffold(),
    );
  }
}
