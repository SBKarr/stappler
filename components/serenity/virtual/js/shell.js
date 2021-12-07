var KawaiJson = function() {
	var init = false;

	var kawaiJson = {
		ontoggle: function (event) {
			var collapsed, target = event.target;
			if (event.target.className == 'collapser') {
				collapsed = target.parentNode.getElementsByClassName('collapsible')[0];
				if (collapsed.parentNode.classList.contains("collapsed"))
					collapsed.parentNode.classList.remove("collapsed");
				else
					collapsed.parentNode.classList.add("collapsed");
			}
		},
		addslashes: function (str) {
			return (str + '').replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');
		},
		recursive: function (parent, json, isLast) {
			var ellipsis = document.createElement('span');
			ellipsis.className = "ellipsis";
			if (Array.isArray(json)) {
				this.addTextNode(parent, '[');
				parent.appendChild(ellipsis);
			}
			else if (typeof json === 'object' && json) {
				this.addTextNode(parent, '{');
				parent.appendChild(ellipsis);
			}

			var ul = document.createElement('ul');
			ul.className = "obj collapsible";
			
			if (json != null) {
				var keys = Object.keys(json);
				for (var keys = Object.keys(json), i = 0, end = keys.length; i < end; i++) {
					var key = keys[i];
					var li = document.createElement('li');
					var keySetted = true;
					if (!isNaN(parseFloat(key)) && isFinite(key)) {
						keySetted = false;
					}
					if (keySetted) {
						var keySpan = document.createElement('span');
						keySpan.className = "property";
						keySpan.innerHTML = '<span class="quote">"</span>' + key + '<span class="quote">"</span>';
						li.appendChild(keySpan);
					}
					var valSpan = document.createElement('span');

					if (typeof json[key] !== 'object' || json[key] == null) {
						if (keySetted)
							this.addTextNode(li, ': ');
						switch (typeof json[key]) {
							case 'boolean':
								valSpan.className = 'type-boolean';
								valSpan.innerHTML = json[key];
								break;
							case 'object':
								valSpan.className = 'type-null';
								valSpan.innerHTML = 'null';
								break;
							case 'number':
								valSpan.className = 'type-number';
								valSpan.innerHTML = json[key];
								break;
							case 'string':
								valSpan.className = 'type-string';
								valSpan.innerHTML = '<span class="quote">"</span>' + this.addslashes(json[key]) + '<span class="quote">"</span>';
								break;
						}
						li.appendChild(valSpan);
						if (i + 1 < end)
							this.addTextNode(li, ',');
						ul.appendChild(li);
					}
					else {
						var collapser = document.createElement('div');
						collapser.className = "collapser";

						if (Array.isArray(json[key])) {
							this.addTextNode(li, ': ');
							li.appendChild(collapser);
							this.recursive(li, json[key], i + 1 >= end);
							ul.appendChild(li);
						}
						else {
							if (keySetted)
								this.addTextNode(li, ': ');
							li.appendChild(collapser);
							this.recursive(li, json[key], i + 1 >= end);
							ul.appendChild(li);
						}

					}
				}

				if (Object.keys(json).length > 0)
					parent.appendChild(ul);
			}

			if (Array.isArray(json)) {
				if (!isLast) {
					this.addTextNode(parent, '],');
				}
				else {
					this.addTextNode(parent, ']');
				}
			}
			else if (typeof json === 'object' && json) {
				if (!isLast) {
					this.addTextNode(parent, '},');
				}
				else {
					this.addTextNode(parent, '}');
				}
			}
		},
		addTextNode: function (parent, text) {
			var el = document.createTextNode(text);
			parent.appendChild(el);
		},
		parse: function (container, json) {
			this.recursive(container, json, true);
		},
	};
	
	return function(container, json) {
		if (init == false) {
			document.body.addEventListener('click', kawaiJson.ontoggle, false);
			init = true;
		}
		kawaiJson.parse(container, json);
	};
}();

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

function init(consoleTarget) {
	document.getElementById("main").style.visibility = "hidden";
	ShellHistory(document.getElementById("input"), send);

	var fileUpload = document.getElementById("fileSelector");
	fileUpload.addEventListener("change", function(event) {
		upload(event.target.files[0]);
	}, false);

	wsaddress = wsaddress + consoleTarget;
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

function connect() {
	ws = new WebSocket(wsaddress);
	ws.onopen = function(event) {
		document.getElementById("main").style.visibility = "visible";
		document.getElementById("login").style.display = "none";
		document.getElementById("main").style.display = "block";
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
		document.getElementById("login").style.display = "block";
		document.getElementById("main").style.display = "none";
	}
	return false;
}

function closeSocket() {
	ws.close();
	return false;
}

function ShellHistory(node, callback) {
	var history = {
		log : [],
		pos : 0
	}

	history["add"] = function(str) {
		history.log[history.log.length] = str;
		history.pos = history.log.length;
	};
	history["next"] = function() {
		if (history.log.length == 0) {
			return "";
		}
		if (history.pos <= 0) {
			history.pos = -1;
			return "";
		}
		history.pos --;
		return history.log[history.pos];
	};
	history["prev"] = function() {
		if (history.log.length == 0) {
			return "";
		}
		if (history.pos >= history.log.length - 1) {
			history.pos = history.log.length;
			return "";
		}
		history.pos ++;
		return history.log[history.pos];
	};

	node.onkeydown = function(ev) {
		if (ev.keyCode == '38') {
			node.value = history.next();
		} else if (ev.keyCode == '40') {
			node.value = history.prev();
		} else if (ev.keyCode == '13') {
			if (node.value != "") {
				history.add(node.value);
				callback(node);
			}
		}
	}
};

function SaveEditorContent(editor) {
	var xhr = new XMLHttpRequest();
	xhr.open("POST", window.location.pathname, true);

	xhr.setRequestHeader("Content-type", "application/markdown");
	
	xhr.onreadystatechange = function() {//Вызывает функцию при смене состояния.
		if (xhr.readyState == XMLHttpRequest.DONE && xhr.status == 200) {
			var jsonResponse = JSON.parse(xhr.responseText);
			if (jsonResponse.OK) {
				window.location.reload();
			}
		}
	}
	xhr.send(editor.value());
}

var simplemde = null;

function ToggleEditor() {
	var editor = document.getElementById("editblock")
	var source = document.getElementById("sourceblock")
	
	if (SimpleMDE) {
		var editorButtons = [
			"bold", "italic", "heading", "|", "code", "unordered-list", "ordered-list", "|", "link",
			{
				name: "toc",
				action: SimpleMDE.drawHorizontalRule,
				className: "fa fa-list-alt",
				title: "Insert TOC"
			},
			"|",
			{
				name: "save",
				action: function saveContent(editor) {
					SaveEditorContent(editor)
				},
				className: "fa fa-floppy-o",
				title: "Save",
			},
			{
				name: "cancel",
				action: function cancel(editor) {
					var editor = document.getElementById("editblock")
					var source = document.getElementById("sourceblock")
					if (!editor.classList.contains("editorclosed")) {
						editor.classList.add("editorclosed");
						if (simplemde != null) {
							simplemde.toTextArea();
							simplemde = null;
						}
						source.classList.remove("editorclosed");
					}
				},
				className: "fa fa-times",
				title: "Cancel",
			},
		];

		if (editor.classList.contains("editorclosed")) {
			editor.classList.remove("editorclosed");
			if (simplemde == null) {
				simplemde = new SimpleMDE({
					element: document.getElementById("editor"),
					spellChecker: false,
					toolbar: editorButtons,
					insertTexts: {
						horizontalRule: ["{{TOC}}"],
					},
					indentWithTabs: true,
					tabSize: 4,
					renderingConfig: {
						codeSyntaxHighlighting: true
					}
				})
			}
			source.classList.add("editorclosed");
		} else {
			source.classList.remove("editorclosed");
			editor.classList.add("editorclosed");
			if (simplemde != null) {
				simplemde.toTextArea();
				simplemde = null;
			}
		}
	}
	return false;
}

function EditorDownShortcuts(e) {
	if (e.ctrlKey && e.which == 83) {
		e.stopPropagation();
		e.preventDefault();  
		e.returnValue = false;
		e.cancelBubble = true;
		return false;
	}
}

function EditorUpShortcuts(e) {
	if (e.ctrlKey && e.which == 83) {
		if (simplemde != null) {
			SaveEditorContent(simplemde)
		}
		return false;
	} else if (e.ctrlKey && e.which == 69) {
		if (simplemde == null) {
			ToggleEditor()
		}
		return false;
	} else if (e.which == 27) {
		if (simplemde != null) {
			ToggleEditor()
		}
		return false;
	}
}

document.addEventListener('keydown', EditorDownShortcuts);
document.addEventListener('keyup', EditorUpShortcuts);

document.addEventListener('copy', (event) => {
	var data = document.getSelection();
	
	if (data.anchorNode.classList.contains('hljs-ln-line') || data.anchorNode.classList.contains('hljs-ln')) {
		var str = data.toString()
		str = str.replace(/\n\t\n/g, '\n');
		str = str.replace(/\n \n/g, '\n\n');
		
		event.clipboardData.setData('text', str);
		event.preventDefault();
	}
});
