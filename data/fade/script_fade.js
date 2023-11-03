var Socket;

const pickr = Pickr.create({
    el: '.color-picker',
    theme: 'classic', // or 'monolith', or 'nano'

    swatches: [
        'rgba(255, 0, 0, 1)',
        'rgba(0, 255, 0, 1)',
        'rgba(0, 0, 255, 1)',
        'rgba(255, 0, 119, 1)',
        'rgba(37, 13, 215, 1)',
        'rgba(33, 150, 243, 1)',
        'rgba(3, 255, 180, 1)',
        'rgba(139, 195, 74, 1)',
        'rgba(205, 220, 57, 1)',
        'rgba(255, 193, 7, 1)',
        'rgba(126, 95, 3, 1)',
        'rgba(255, 117, 0, 1)',
        'rgba(255, 255, 255, 1)',
        'rgba(0, 0, 0, 1;)'
    ],

    components: {

        // Main components
        preview: true,
        opacity: false,
        hue: true,

        // Input / output Options
        interaction: {
            input: true,
            clear: true,
            save: false
        }
    }
});

pickr.on('change', color => {
    let color_hex;
    if(color == null){
        color_hex = "#000000";
    }else{
    color_hex = color.toHEXA().toString();
    }
   color_changed(color_hex);
})

let color_picker_button = document.querySelector(".pcr-button");
let panel = document.getElementById("panel");
let slider = document.getElementById("LED_INTENSITY");
let slider_value = document.getElementById("LED_VALUE");

slider.addEventListener("input", slider_changed);

function init(){
    //create Websocket
    Socket = new WebSocket('ws://' + window.location.hostname +':81/');

    color_picker_button.style.setProperty("height", "5em");
    color_picker_button.style.setProperty("width", "5em");
    
    //run on Client Request
    Socket.onmessage = function(event){
        //get Json Obj from sendet Data
        var object = JSON.parse(event.data);
        var type = object.type;
        if(type.localeCompare("LED_INTENSITY") == 0){
            let l_led_intensity = object.value;
            console.log(l_led_intensity);
            slider.value = l_led_intensity;
            slider_value.innerHTML = l_led_intensity;
        }
        else if(type.localeCompare("LED_COLOR") == 0){
           let l_led_color = object.value;
           console.log(l_led_color);
           color_picker_button.style.setProperty("--pcr-color", l_led_color);
        }
       
    }
}
//send staus 
function color_changed(color){
    //create status_json
   var msg = {
    type: "LED_COLOR",
    value: color
   }
    //parse staus in Json Obj and send to Socket
    Socket.send(JSON.stringify(msg));
    console.log(msg);
}

function slider_changed(){
    var l_led_intensity = slider.value;
    console.log(l_led_intensity);
    var msg = {
        type: "LED_INTENSITY",
        value: l_led_intensity
    }
    Socket.send(JSON.stringify(msg));
    
}

//run init at start
window.onload = function(event){
    init();
}