# rcswitch-mqtt

In it's current state, this sketch connects to the specified MQTT broker and subscribes to a topic and it's children.

Once it gets a message it will call RCSwitch (https://github.com/sui77/rc-switch) with the first parameter set to 1 and the second parameter set to the topic name, and turn it on or off depending on the payload (0 or 1).

By default, the subscribed topic is 'rfswitch/+' so, for some examples:

sending '1' to rfswitch/1 will turn switch 1 ON (RCSwitch.switchOn(1, 1);

sending '0' to rfswitch/3 will turn switch 3 OFF (RCSwitch.switchOff(1, 3);

On startup, and every hour, it will publish a message to a 'messages' topic.