# MyProject

Developed with Unreal Engine 4

```shell
curl -i -X GET https://0flacbdme3.execute-api.ap-southeast-2.amazonaws.com/dev/

curl -i -X POST https://0flacbdme3.execute-api.ap-southeast-2.amazonaws.com/dev/ -H 'Content-Type: application/json' -d '{ "device_id": "UEtest", "created_timestamp": 19947499999, "longitude": "778.107479", "latitude": "-99.0", "elevation": "268.373724"}'

curl -i -X DELETE https://0flacbdme3.execute-api.ap-southeast-2.amazonaws.com/dev/ -H 'Content-Type: application/json' -d '{ "device_id": "test_device2", "created_timestamp": 1647876557}'
```