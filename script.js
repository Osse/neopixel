var websock;
var leds;
var timer;

function element(id) {
    return document.getElementById(id);
}

function updateColor(i) {
    var led = leds[i];
    var colorArray = [ led.red.value, led.green.value, led.blue.value ];
    led.color.style.backgroundColor = "rgb(" + colorArray.join(",") + ")";
}

function send(slider) {
    var id = slider.id;
    if (websock)
        websock.send(id + ":" + element(id).value);
    updateColor(id.slice(-1));
}

function handleUpdate(evt) {
    var kvs = evt.data.split(",");
    for (kv of kvs) {
        [key, value] = kv.split(":");
        element(key).value = value;
    }
    updateColor(0);
    updateColor(1);
    element("status").innerHTML = "Refreshed!";
}

function start_settings() {
    if (window.location.hostname)
    {
        websock = new WebSocket('ws://' + window.location.hostname + ':81/');
        websock.onmessage = handleUpdate;
    }

    // Array of objects, each with four members: the sliders and the color div
    leds = [
        {
            red: element("red_0"),
            green: element("green_0"),
            blue: element("blue_0"),
            color: element("color_0")
        },
        {
            red: element("red_1"),
            green: element("green_1"),
            blue: element("blue_1"),
            color: element("color_1")
        }
    ];

    // To update the color divs on Refresh
    updateColor(0);
    updateColor(1);
}
