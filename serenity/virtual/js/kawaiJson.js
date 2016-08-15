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

