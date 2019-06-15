function escape(str) {
    var tagsToReplace = {
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;'
    };
    return str.replace(/[&<>]/g, function(tag) {
        return tagsToReplace[tag] || tag;
    });
};

function upload(file, url, name) {
	var xhr = new XMLHttpRequest();
	var data = new FormData();
	data.append(name, file);

	var output = document.getElementById("output");
	var node = document.createElement("p");
	node = output.insertBefore(node, output.firstChild);
	node.innerHTML = "Upload started";

	xhr.onload = xhr.onerror = function() {
		if (this.status == 200) {
			push(this.response);
		} else {
			push("Upload error: " + this.status);
		}
		output.removeChild(node);
	};

	xhr.upload.onprogress = function(event) {
		node.innerHTML = event.loaded + ' / ' + event.total;
	};

	xhr.open("POST", url, true);
	xhr.send(data);
}

function uploadWithData(data) {
	document.getElementById('fileSelector').click();
}

var ws;
var wsaddress = ((window.location.protocol == "https:") ? "wss:"
		: "ws:")
		+ "//" + window.location.host + window.location.pathname
if ((typeof (WebSocket) == 'undefined')
		&& (typeof (MozWebSocket) != 'undefined')) {
	WebSocket = MozWebSocket;
}
function send(input) {
	if (ws) {
		ws.send(input.value);
		input.value = "";
	}
}
function init() {
	document.getElementById("main").style.visibility = "hidden";
	ShellHistory(document.getElementById("input"), send);

	var fileUpload = document.getElementById("fileSelector");
	fileUpload.addEventListener("change", function(event) {
		upload(event.target.files[0]);
	}, false);
}

function push(data) {
	var output = document.getElementById("output");
	var node = document.createElement("p");
	if (data[0] == '{' || data[0] == '[') {
		KawaiJson(node, JSON.parse(data));
	} else if (data[0] == '<') {
		node.innerHTML = data;
	} else {
		node.innerHTML = escape(data);
	}
	output.insertBefore(node, output.firstChild);
}

function connect(form) {
	var nameEl = document.getElementById("ws_name");
	var passwdEl = document.getElementById("ws_passwd");

	var name = nameEl.value;
	var passwd = passwdEl.value;

	nameEl.value = "";
	passwdEl.value = "";

	ws = new WebSocket(wsaddress + "?name=" + name + "&passwd="
			+ passwd);
	ws.onopen = function(event) {
		document.getElementById("login").style.visibility = "hidden";
		document.getElementById("main").style.visibility = "visible";
		document.getElementById("connected").innerHTML = "Connected to WebSocket server";
		document.getElementById("output").innerHTML = "";
	};
	ws.onmessage = function(event) {
		if (event.data[0] != ':') {
			push(event.data);
		} else if (event.data.startsWith(':upload:')) {
			uploadWithData(JSON.parse(event.data.substr(8)));
		}
	};
	ws.onerror = function(event) {
		console.log(event);
	};
	ws.onclose = function(event) {
		ws = null;
		console.log(event);
		document.getElementById("login").style.visibility = "visible";
		document.getElementById("main").style.visibility = "hidden";
		document.getElementById("connected").innerHTML = "Connection Closed";
	}
	return false;
}