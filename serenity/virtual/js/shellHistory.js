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
