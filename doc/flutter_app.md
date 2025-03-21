# Flutter App Development Guide for Aquarium LED Controller

This document provides instructions for developing a Flutter application to control the aquarium LED system via WiFi connection.

## Application Structure

The Flutter app consists of several main components:

1. **API Service**: Handles HTTP communication with the ESP32
2. **Main UI**: Dashboard for light control
3. **Profile Settings UI**: For configuring light profiles
4. **Time Settings UI**: For setting time ranges
5. **Local Storage**: For storing settings and preferences

## Required Flutter Packages

```yaml
dependencies:
  flutter:
    sdk: flutter
  http: ^0.13.5
  shared_preferences: ^2.1.1
  provider: ^6.0.5
  json_annotation: ^4.8.1
  intl: ^0.18.1
  flutter_colorpicker: ^1.0.3
  charts_flutter: ^0.12.0
```

## HTTP Communication

### API Service Class

Here's a sample implementation of an API service class to communicate with the ESP32:

```dart
import 'dart:convert';
import 'package:http/http.dart' as http;

class AquariumApiService {
  String baseUrl;
  final http.Client client = http.Client();
  
  AquariumApiService({required this.baseUrl});
  
  // Get current profile
  Future<Map<String, dynamic>> getCurrentProfile() async {
    final response = await client.get(Uri.parse('$baseUrl/api/profile'));
    if (response.statusCode == 200) {
      return json.decode(response.body);
    } else {
      throw Exception('Failed to load profile');
    }
  }
  
  // Get profile by type
  Future<Map<String, dynamic>> getProfile(String type) async {
    final response = await client.get(Uri.parse('$baseUrl/api/profile?type=$type'));
    if (response.statusCode == 200) {
      return json.decode(response.body);
    } else {
      throw Exception('Failed to load profile');
    }
  }
  
  // Set profile
  Future<bool> setProfile(String type, Map<String, dynamic> profile) async {
    final data = {
      'type': type,
      'profile': profile
    };
    
    final response = await client.post(
      Uri.parse('$baseUrl/api/profile'),
      headers: {'Content-Type': 'application/json'},
      body: json.encode(data)
    );
    
    return response.statusCode == 200;
  }
  
  // Get time ranges
  Future<Map<String, dynamic>> getTimeRanges() async {
    final response = await client.get(Uri.parse('$baseUrl/api/timeranges'));
    if (response.statusCode == 200) {
      return json.decode(response.body);
    } else {
      throw Exception('Failed to load time ranges');
    }
  }
  
  // Set time ranges
  Future<bool> setTimeRanges(Map<String, dynamic> timeRanges) async {
    final response = await client.post(
      Uri.parse('$baseUrl/api/timeranges'),
      headers: {'Content-Type': 'application/json'},
      body: json.encode(timeRanges)
    );
    
    return response.statusCode == 200;
  }
  
  // Manual LED control
  Future<bool> controlLed(String led, int value) async {
    final data = {
      'led': led,
      'value': value
    };
    
    final response = await client.post(
      Uri.parse('$baseUrl/api/manual'),
      headers: {'Content-Type': 'application/json'},
      body: json.encode(data)
    );
    
    return response.statusCode == 200;
  }
  
  // Get current time
  Future<Map<String, dynamic>> getCurrentTime() async {
    final response = await client.get(Uri.parse('$baseUrl/api/time'));
    if (response.statusCode == 200) {
      return json.decode(response.body);
    } else {
      throw Exception('Failed to load current time');
    }
  }
  
  // Get operation mode
  Future<String> getMode() async {
    final response = await client.get(Uri.parse('$baseUrl/api/mode'));
    if (response.statusCode == 200) {
      final data = json.decode(response.body);
      return data['mode'];
    } else {
      throw Exception('Failed to load mode');
    }
  }
  
  // Set operation mode
  Future<bool> setMode(String mode) async {
    final data = {'mode': mode};
    
    final response = await client.post(
      Uri.parse('$baseUrl/api/mode'),
      headers: {'Content-Type': 'application/json'},
      body: json.encode(data)
    );
    
    return response.statusCode == 200;
  }
}
```

### JSON Data Formats

Communication with the ESP32 uses JSON format. Here are the formats used:

#### Profile Setting:
```json
{
  "type": "morning|midday|evening|night|current",
  "profile": {
    "royalBlue": 100,
    "blue": 150,
    "uv": 50,
    "violet": 50,
    "red": 150,
    "green": 100,
    "white": 200
  }
}
```

#### Manual Control:
```json
{
  "led": "royalBlue|blue|uv|violet|red|green|white",
  "value": 0-255
}
```

#### Time Ranges:
```json
{
  "morningStart": 7,
  "middayStart": 10,
  "eveningStart": 17,
  "nightStart": 21
}
```

#### Mode:
```json
{
  "mode": "manual|auto"
}
```

## Discovering ESP32 on Network

Finding the ESP32 on the local network can be implemented in several ways:

1. **Manual IP entry**: Allow users to enter the ESP32's IP address manually
2. **mDNS discovery**: If the ESP32 implements mDNS (Bonjour), you can discover it by name
3. **Network scan**: Scan common ports on the local network for the ESP32 server

Here's a simple example of manual IP configuration:

```dart
import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:provider/provider.dart';

class SettingsScreen extends StatefulWidget {
  @override
  _SettingsScreenState createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  final _ipController = TextEditingController();
  final _prefs = SharedPreferences.getInstance();
  
  @override
  void initState() {
    super.initState();
    _loadSavedIP();
  }
  
  _loadSavedIP() async {
    final prefs = await _prefs;
    setState(() {
      _ipController.text = prefs.getString('esp32_ip') ?? '192.168.4.1';
    });
  }
  
  _saveIP() async {
    final prefs = await _prefs;
    await prefs.setString('esp32_ip', _ipController.text);
    
    // Update the API service with the new base URL
    final apiService = Provider.of<AquariumApiService>(context, listen: false);
    apiService.baseUrl = 'http://${_ipController.text}';
  }
  
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text('Settings')),
      body: Padding(
        padding: EdgeInsets.all(16.0),
        child: Column(
          children: [
            TextField(
              controller: _ipController,
              decoration: InputDecoration(
                labelText: 'ESP32 IP Address',
                hintText: 'e.g. 192.168.1.100',
              ),
              keyboardType: TextInputType.text,
            ),
            SizedBox(height: 16),
            ElevatedButton(
              onPressed: () {
                _saveIP();
                ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(content: Text('IP Address saved'))
                );
              },
              child: Text('Save'),
            ),
          ],
        ),
      ),
    );
  }
}
```

## UI Implementation

### Main Dashboard

Example of a dashboard implementation:

```dart
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

class DashboardScreen extends StatefulWidget {
  @override
  _DashboardScreenState createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  bool _isManualMode = false;
  Map<String, int> _ledValues = {
    'royalBlue': 0,
    'blue': 0,
    'uv': 0,
    'violet': 0,
    'red': 0,
    'green': 0,
    'white': 0,
  };
  bool _isLoading = true;
  
  @override
  void initState() {
    super.initState();
    _loadCurrentState();
  }
  
  Future<void> _loadCurrentState() async {
    setState(() => _isLoading = true);
    
    try {
      final apiService = Provider.of<AquariumApiService>(context, listen: false);
      
      // Load current mode
      final mode = await apiService.getMode();
      
      // Load current profile
      final profile = await apiService.getCurrentProfile();
      
      setState(() {
        _isManualMode = mode == 'manual';
        if (profile.containsKey('royalBlue')) _ledValues['royalBlue'] = profile['royalBlue'];
        if (profile.containsKey('blue')) _ledValues['blue'] = profile['blue'];
        if (profile.containsKey('uv')) _ledValues['uv'] = profile['uv'];
        if (profile.containsKey('violet')) _ledValues['violet'] = profile['violet'];
        if (profile.containsKey('red')) _ledValues['red'] = profile['red'];
        if (profile.containsKey('green')) _ledValues['green'] = profile['green'];
        if (profile.containsKey('white')) _ledValues['white'] = profile['white'];
      });
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Error loading data: ${e.toString()}'))
      );
    } finally {
      setState(() => _isLoading = false);
    }
  }
  
  Future<void> _toggleMode(bool value) async {
    try {
      final apiService = Provider.of<AquariumApiService>(context, listen: false);
      final success = await apiService.setMode(value ? 'manual' : 'auto');
      
      if (success) {
        setState(() => _isManualMode = value);
      } else {
        throw Exception('Failed to set mode');
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Error setting mode: ${e.toString()}'))
      );
    }
  }
  
  Future<void> _setLedValue(String led, int value) async {
    try {
      final apiService = Provider.of<AquariumApiService>(context, listen: false);
      final success = await apiService.controlLed(led, value);
      
      if (success) {
        setState(() => _ledValues[led] = value);
      } else {
        throw Exception('Failed to set LED value');
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Error setting LED: ${e.toString()}'))
      );
    }
  }
  
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Aquarium LED Controller'),
        actions: [
          IconButton(
            icon: Icon(Icons.settings),
            onPressed: () => Navigator.pushNamed(context, '/settings'),
          ),
        ],
      ),
      body: _isLoading
          ? Center(child: CircularProgressIndicator())
          : SingleChildScrollView(
              padding: EdgeInsets.all(16.0),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  // Mode toggle
                  Card(
                    child: Padding(
                      padding: EdgeInsets.all(16.0),
                      child: Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          Text(
                            'Mode: ${_isManualMode ? 'Manual' : 'Auto'}',
                            style: TextStyle(fontSize: 18),
                          ),
                          Switch(
                            value: _isManualMode,
                            onChanged: _toggleMode,
                          ),
                        ],
                      ),
                    ),
                  ),
                  
                  SizedBox(height: 16),
                  
                  // Manual controls
                  if (_isManualMode) ...[
                    Text(
                      'Manual LED Control',
                      style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
                    ),
                    
                    SizedBox(height: 8),
                    
                    // LED sliders
                    _buildLedSlider('Royal Blue', 'royalBlue', Colors.blue[900]!),
                    _buildLedSlider('Blue', 'blue', Colors.blue),
                    _buildLedSlider('UV', 'uv', Colors.purple[900]!),
                    _buildLedSlider('Violet', 'violet', Colors.purple),
                    _buildLedSlider('Red', 'red', Colors.red),
                    _buildLedSlider('Green', 'green', Colors.green),
                    _buildLedSlider('White', 'white', Colors.white),
                  ],
                  
                  SizedBox(height: 16),
                  
                  // Profile management buttons
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                    children: [
                      ElevatedButton.icon(
                        icon: Icon(Icons.lightbulb_outline),
                        label: Text('Edit Profiles'),
                        onPressed: () => Navigator.pushNamed(context, '/profiles'),
                      ),
                      ElevatedButton.icon(
                        icon: Icon(Icons.access_time),
                        label: Text('Edit Time Ranges'),
                        onPressed: () => Navigator.pushNamed(context, '/timeranges'),
                      ),
                    ],
                  ),
                  
                  SizedBox(height: 16),
                  
                  // Refresh button
                  ElevatedButton.icon(
                    icon: Icon(Icons.refresh),
                    label: Text('Refresh'),
                    onPressed: _loadCurrentState,
                  ),
                ],
              ),
            ),
    );
  }
  
  Widget _buildLedSlider(String label, String key, Color color) {
    return Card(
      margin: EdgeInsets.symmetric(vertical: 8),
      child: Padding(
        padding: EdgeInsets.all(12.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              label,
              style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
            ),
            Row(
              children: [
                Container(
                  width: 24,
                  height: 24,
                  decoration: BoxDecoration(
                    color: color.withOpacity((_ledValues[key] ?? 0) / 255),
                    shape: BoxShape.circle,
                    border: Border.all(color: Colors.grey),
                  ),
                ),
                SizedBox(width: 8),
                Expanded(
                  child: Slider(
                    value: (_ledValues[key] ?? 0).toDouble(),
                    min: 0,
                    max: 255,
                    divisions: 255,
                    label: '${_ledValues[key]}',
                    onChanged: (value) {
                      setState(() => _ledValues[key] = value.toInt());
                    },
                    onChangeEnd: (value) {
                      _setLedValue(key, value.toInt());
                    },
                  ),
                ),
                SizedBox(width: 8),
                Text('${_ledValues[key]}'),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
```

## Error Handling

Implement robust error handling for network issues:

```dart
import 'dart:io';
import 'package:flutter/material.dart';

Future<void> performApiCall(BuildContext context, Future<void> Function() apiCall) async {
  try {
    await apiCall();
  } on SocketException catch (_) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text('Connection Error'),
        content: Text('Could not connect to the aquarium controller. Please check that the device is powered on and connected to the same network.'),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: Text('OK'),
          ),
        ],
      ),
    );
  } catch (e) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text('Error'),
        content: Text('An error occurred: ${e.toString()}'),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: Text('OK'),
          ),
        ],
      ),
    );
  }
}
```

## Data Caching

For a better user experience, implement local caching of profiles and settings:

```dart
import 'dart:convert';
import 'package:shared_preferences/shared_preferences.dart';

class ProfileCache {
  static Future<void> saveProfile(String type, Map<String, dynamic> profile) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('profile_$type', json.encode(profile));
  }
  
  static Future<Map<String, dynamic>?> getProfile(String type) async {
    final prefs = await SharedPreferences.getInstance();
    final data = prefs.getString('profile_$type');
    if (data != null) {
      return json.decode(data);
    }
    return null;
  }
  
  static Future<void> saveTimeRanges(Map<String, dynamic> timeRanges) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('time_ranges', json.encode(timeRanges));
  }
  
  static Future<Map<String, dynamic>?> getTimeRanges() async {
    final prefs = await SharedPreferences.getInstance();
    final data = prefs.getString('time_ranges');
    if (data != null) {
      return json.decode(data);
    }
    return null;
  }
}
```

## Connection Management

It's important to manage the connection state with the ESP32:

```dart
import 'dart:async';
import 'package:flutter/material.dart';

class ConnectionManager extends ChangeNotifier {
  bool _isConnected = false;
  String _ipAddress = '';
  Timer? _connectionCheckTimer;
  final AquariumApiService _apiService;
  
  ConnectionManager(this._apiService) {
    // Start periodic connection checks
    _connectionCheckTimer = Timer.periodic(Duration(seconds: 10), (_) {
      checkConnection();
    });
  }
  
  bool get isConnected => _isConnected;
  String get ipAddress => _ipAddress;
  
  void setIpAddress(String ip) {
    _ipAddress = ip;
    _apiService.baseUrl = 'http://$ip';
    notifyListeners();
    checkConnection();
  }
  
  Future<void> checkConnection() async {
    try {
      // Try to get current time as a simple connection test
      await _apiService.getCurrentTime();
      _isConnected = true;
    } catch (e) {
      _isConnected = false;
    }
    notifyListeners();
  }
  
  @override
  void dispose() {
    _connectionCheckTimer?.cancel();
    super.dispose();
  }
}
```

## Deployment

To build the application for different platforms:

1. Build APK for Android:
   ```bash
   flutter build apk --release
   ```

2. Build App Bundle for Google Play:
   ```bash
   flutter build appbundle
   ```

3. Build for iOS:
   ```bash
   flutter build ios --release
   ```

## Development Tips

1. **Responsive UI**: Ensure the app works well on different screen sizes
2. **Caching**: Store last settings locally
3. **Error Handling**: Handle connection drops and communication errors
4. **Testing**: Test the app with the actual ESP32 device
5. **Background Updates**: Consider implementing a background service to monitor the aquarium 