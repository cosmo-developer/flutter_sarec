// ignore_for_file: always_specify_types
// ignore_for_file: camel_case_types
// ignore_for_file: non_constant_identifier_names

// AUTO GENERATED FILE, DO NOT EDIT.
//
// Generated by `package:ffigen`.
import 'dart:ffi' as ffi;

// ignore: depend_on_referenced_packages
import 'package:ffi/ffi.dart';

/// Bindings for `src/flutter_sarec.h`.
///
/// Regenerate bindings with `flutter pub run ffigen --config ffigen.yaml`.
///
class FlutterSarecBindings {
  /// Holds the symbol lookup function.
  final ffi.Pointer<T> Function<T extends ffi.NativeType>(String symbolName)
      _lookup;

  /// The symbols are looked up in [dynamicLibrary].
  FlutterSarecBindings(ffi.DynamicLibrary dynamicLibrary)
      : _lookup = dynamicLibrary.lookup;

  /// The symbols are looked up with [lookup].
  FlutterSarecBindings.fromLookup(
      ffi.Pointer<T> Function<T extends ffi.NativeType>(String symbolName)
          lookup)
      : _lookup = lookup;

  /// A very short-lived native function.
  ///
  /// For very short-lived functions, it is fine to call them on the main isolate.
  /// They will block the Dart execution while running the native function, so
  /// only do this for native functions which are guaranteed to be short-lived.
  int sum(
    int a,
    int b,
  ) {
    return _sum(
      a,
      b,
    );
  }

  late final _sumPtr =
      _lookup<ffi.NativeFunction<ffi.IntPtr Function(ffi.IntPtr, ffi.IntPtr)>>(
          'sum');
  late final _sum = _sumPtr.asFunction<int Function(int, int)>();

  /// A longer lived native function, which occupies the thread calling it.
  ///
  /// Do not call these kind of native functions in the main isolate. They will
  /// block Dart execution. This will cause dropped frames in Flutter applications.
  /// Instead, call these native functions on a separate isolate.
  int sum_long_running(
    int a,
    int b,
  ) {
    return _sum_long_running(
      a,
      b,
    );
  }

  late final _sum_long_runningPtr =
      _lookup<ffi.NativeFunction<ffi.IntPtr Function(ffi.IntPtr, ffi.IntPtr)>>(
          'sum_long_running');
  late final _sum_long_running =
      _sum_long_runningPtr.asFunction<int Function(int, int)>();

  int create_sarec_client() {
    return _create_sarec_client();
  }

  late final _create_sarec_clientPtr =
      _lookup<ffi.NativeFunction<ffi.IntPtr Function()>>("CreateSarecClient");

  late final _create_sarec_client =
      _create_sarec_clientPtr.asFunction<int Function()>();

  int start(int client, String fileName) {
    return _start(client, fileName.toNativeUtf8().cast());
  }

  late final _startPtr = _lookup<
      ffi.NativeFunction<
          ffi.IntPtr Function(ffi.IntPtr, ffi.Pointer<ffi.Uint8>)>>('Start');

  late final _start =
      _startPtr.asFunction<int Function(int, ffi.Pointer<ffi.Uint8>)>();

  int stop(int client) {
    return _stop(client);
  }

  late final _stopPtr =
      _lookup<ffi.NativeFunction<ffi.IntPtr Function(ffi.IntPtr)>>('Stop');

  late final _stop = _stopPtr.asFunction<int Function(int)>();

  int pause(int client) {
    return _pause(client);
  }

  late final _pausePtr =
      _lookup<ffi.NativeFunction<ffi.IntPtr Function(ffi.IntPtr)>>('Pause');

  late final _pause = _pausePtr.asFunction<int Function(int)>();

  int resume(int client) {
    return _resume(client);
  }

  late final _resumePtr =
      _lookup<ffi.NativeFunction<ffi.IntPtr Function(ffi.IntPtr)>>('Resume');

  late final _resume = _resumePtr.asFunction<int Function(int)>();

  int is_recording(int client) {
    return _is_recording(client);
  }

  late final _is_recordingPtr =
      _lookup<ffi.NativeFunction<ffi.IntPtr Function(ffi.IntPtr)>>(
          'IsRecording');

  late final _is_recording = _is_recordingPtr.asFunction<int Function(int)>();

  int destroy_sarec_client(int client) {
    return _destroy_sarec_client(client);
  }

  late final _destroy_sarec_clientPtr =
      _lookup<ffi.NativeFunction<ffi.IntPtr Function(ffi.IntPtr)>>(
          'DestroySarecClient');

  late final _destroy_sarec_client =
      _destroy_sarec_clientPtr.asFunction<int Function(int)>();
}
