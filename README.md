# SpacesMarkerManager

This is an Unreal Engine plugin designed and written by Kai Hirota for Mojexa Spaces. This plugin is for Unreal Engine 5.0.0.

Features
- 3 Customizable and extensible Location Marker classes implemented as Actors.
   - Static Marker: Space and Time is fixed. Once spawned, it will remain in place until deleted.
   - Temporay Marker: Space is fixed, but has a time-to-live counter which continues to decrement as time passes. It shrinks over time until it eventually self-destructs.
   - Dynamic Marker: Contains a priority queue of data points, where each data point is a pair of coordinate and timestamp. Continues to move to the next location as determined by the queue. In other words, it will move in chronological order with respect to the timestamps. Once it reaches the final location, the counter starts decrementing until it eventually self-destructs.
- SpacesMarkerManager
   - Interface / manager for the location markers. It is implemented as a `UGameInstance`, which guarantees there is always one instance, and it is automatically created and destroyed.
   - Caveat of using `UGameInstance` is that you cannot use other `UGameInstance` since there can only be one. In such case you can combine them into `UGameInstance` or convert them to `UGameInstanceSubsystem`.
   - Handles creation, deletion, coordination of location markers.
   - Maintains connection to DynamoDB and DynamoDB Streams, and allows you to listen to inserts, as well as do a replay of inserts in the last 24 hours.

Dependencies:
- [awsSDK by Siqi Wu](https://www.unrealengine.com/marketplace/en-US/product/aws-dynamodb)
- [CesiumForUnreal](https://github.com/CesiumGS/cesium-unreal)

Example Projects:
- [Blueprint Version](https://github.com/from81/MojexaSampleProject)
- [C++ Version](https://github.com/from81/MojexaSampleProjectC)

## Get Started
1. In the UE Project, enable the two plugins above. When developing, I had the 2 plugins above installed as Engine plugin, not Project plugin.
2. Download this repository, and save it in an Unreal Engine project under `Plugins`. If the UE project is `MyProject`, then this repository should be saved in `MyProject/Plugins/`. So after this step your should have `MyProject/Plugins/SpacesMarkerManager`.
3. Edit `settings.h`
     - For testing, use Docker to spin up a local instance of `DynamoDB` and `DynamoDB Streams`, and set `UseDynamoDBLocal` in `settings.h` to `true`. See [Source/SpacesMarkerManager/Public/Settings.h](Source/SpacesMarkerManager/Public/Settings.h) for how to spin up DynamoDB locally.
     - For production, set `UseDynamoDBLocal` to `false` and configure `AWSAccessKeyId`, `AWSSecretKey`, `SpacesAwsRegion`, `DynamoDBTableName`.
     - When using the plugin in the sample projects listed above, keep the `UseCesiumGeoreference = false;`, since the sample projects do not use Cesium. However, it still lists Cesium as a plugin so make sure to enable the Cesium plugin.
4. Open the UE Project. Go to Edit > Project Settings > Maps & Modes, scroll to the bottom, and change "Game Instance Class" to "MarkerManager".
5. Use this plugin by Blueprint or C++. See examples for more information.

Changelog:

- 25 May 2022: 1.0 of the report attached to the repository in [here](/Doc/report.md).