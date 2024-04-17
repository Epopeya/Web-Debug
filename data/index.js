const canvas = document.querySelector(".panel.map > .content > canvas");
const status_label = document.querySelector(".bar .status");
const msg_text = document.querySelector(".panel.msg .content > code");
const battery_voltage = document.querySelector(".panel.status .content #battery_voltage");

var socket_url = new URL("/ws", window.location.href);
const is_local = socket_url.protocol == "file:";
if (is_local) {
    log("debug mode");
    socket_url = new URL("ws://localhost:1234");
    document.querySelector(".bar .debug").style.display = "block";
}
socket_url.protocol = socket_url.protocol.replace("http", "ws");
console.log("Websocket URL: ", socket_url.href);
const socket = new WebSocket(socket_url.href);

const map_img = new Image();
map_img.src = "map.svg";
map_img.onload = () => {
    map_loaded = true;
};
var map_loaded = false;

let robot_pos;
let robot_rot;
let robot_trot;
let robot_bat;
let robot_route;

function drawMap() {
    const ctx = canvas.getContext("2d");
    ctx.save();
    
    ctx.fillStyle = "#FFFFFF";
    ctx.fillRect(0, 0, 1200, 1200);
    if (map_loaded) {
        ctx.drawImage(map_img, 0, 0, 1200, 1200);
    }

    ctx.translate(600.5, 600.5);
    ctx.scale(0.09*4, -0.09*4);

    ctx.strokeStyle = "#000000";
    ctx.lineWidth = 40;
    ctx.strokeRect(-1500, -1500, 3000, 3000);
    ctx.lineWidth = 40;
    ctx.strokeRect(-500, -500, 1000, 1000);

    if (robot_route != undefined) {
        let from = robot_pos;
        robot_route.forEach((point) => {
            ctx.save();

            ctx.fillStyle = "#22DD22";
            ctx.beginPath();
            ctx.arc(point[0], point[1], 40, 0, 2 * Math.PI);
            ctx.fill();

            ctx.strokeStyle = "#22DD22";
            ctx.lineWidth = 6;
            ctx.beginPath();
            ctx.moveTo(from[0], from[1]);
            ctx.lineTo(point[0], point[1]);
            ctx.stroke();

            ctx.restore();

            from = point;
        });
    }

    if (robot_pos != undefined && robot_rot != undefined) {
        if (robot_trot != undefined) {
            ctx.save();
            ctx.translate(robot_pos[0], robot_pos[1]);
            ctx.rotate(robot_trot - Math.PI/2);
            ctx.strokeStyle = "#2222DD";
            ctx.lineWidth = 20;
            ctx.beginPath();
            ctx.moveTo(0, 0);
            ctx.lineTo(0, 750);
            ctx.stroke();
            ctx.restore();
        }
        ctx.save();
        ctx.fillStyle = "#DD2222";
        ctx.translate(robot_pos[0], robot_pos[1]);
        ctx.rotate(robot_rot - Math.PI/2);
        ctx.fillRect(-150/2, -150/2, 150, 200);
        ctx.strokeStyle = "#DD2222";
        ctx.lineWidth = 40;
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(0, 500);
        ctx.stroke();
        ctx.restore();
    }

    ctx.restore();
}

function refreshStatus() {
    if (robot_bat) {
        battery_voltage.innerText = Math.round(robot_bat * 10) / 10;
        const li = battery_voltage.parentElement;
        if (robot_bat < 6) {
            li.className = "err";
        } else if (robot_bat < 7.2) {
            li.className = "warn";
        } else {
            li.className = "ok";
        }
    }
}

function msg(el) {
    msg_text.appendChild(el);
}

function log(str) {
    console.log(str);
    let p = document.createElement("p");
    p.className = "client";
    p.appendChild(document.createTextNode(str));
    msg(p);
}

function wsOpen(event) {
    log("socket connected");
    status_label.className = "status connected";
}

function wsClose(event) {
    log("socket closed");
    status_label.className = "status disconnected";
}

function wsMessage(event) {
    console.log(event)
    const json = JSON.parse(event.data);
    for (field in json) {
        const data = json[field];
        switch(field) {
            case "msgs":
                for (i in data) {
		    let p = document.createElement("p");
		    p.appendChild(document.createTextNode(data[i]));
                    msg(p);
                }
                break;
            case "pos":
                robot_pos = data;
                break;
            case "rot":
                robot_rot = data;
                break;
            case "trot":
                robot_trot = data;
                break;
            case "bat":
                robot_bat = data;
                break;
            case "route":
                robot_route = data;
                break;
            default:
                log("unknown package \"" + field + "\"");
                break;
        }
    }

    drawMap();
    refreshStatus();
}

drawMap();

socket.addEventListener("open", wsOpen);
socket.addEventListener("close", wsClose);
socket.addEventListener("message", wsMessage);
