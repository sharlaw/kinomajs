//@program
/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
var Pins = require("pins");

// ----------------------------------------------------------------------------------
// Skins
// ----------------------------------------------------------------------------------
const blackSkin = new Skin({fill: "black"});
const iconSkin = new Skin({ texture:new Texture("./icon.png", 1), x:0, y:0, width:256, height:256, aspect:"fit" });

// ----------------------------------------------------------------------------------
// Main screen / UI elements
// ----------------------------------------------------------------------------------

const whiteStyle = new Style({ font:"Arial", size:24, color:"white", horizontal:"center" });

// Main UI container
var MainScreen = Container.template(function($) { return {
	left: 0, right: 0, top: 0, bottom: 0, skin: blackSkin, active:true,
	contents: [
		Content($, { center:0, middle:0, skin:iconSkin }),
	]
}});

Handler.Bind("/network/interface/remove", class extends Behavior {
	onInvoke(handler, message) {
		application.invoke(new Message("xkpr://shell/close?id=" + application.id));
	}
});

// ----------------------------------------------------------------------------------
// Application
// ----------------------------------------------------------------------------------

const PINMUX_DISCONNECTED = 0;
const PINMUX_POWER = 1;
const PINMUX_GROUND = 2;
const PINMUX_ANALOG = 3;
const PINMUX_DIGITAL_IN = 4;
const PINMUX_DIGITAL_OUT = 5;
const PINMUX_I2C_CLK = 6
const PINMUX_I2C_SDA = 7;
const PINMUX_SERIAL_RX = 8
const PINMUX_SERIAL_TX = 9;
const PINMUX_PWM = 10;

const TYPE_DISCONNECTED = "Disconnected";
const TYPE_POWER = "Power";
const TYPE_GROUND = "Ground";
const TYPE_ANALOG = "Analog";
const TYPE_DIGITAL = "Digital";
const TYPE_I2C = "I2C"
const TYPE_SERIAL = "Serial";
const TYPE_PWM = "PWM";

let handlers = {
	pinExplorerStart(helper, query) {
		Pins.invoke("getPinMux", pinmux => {
//			trace(JSON.stringify(pinmux, null, " ") + "\n");
			let explorer = {};
			explorer.leftVoltage = pinmux.leftVoltage;
			explorer.rightVoltage = pinmux.rightVoltage;
			let leftPins = pinmux.leftPins;
			for (let pin = 0, c = leftPins.length; pin < c; pin++) {
				helper.pinExplorerAddPin(explorer, 51 + pin, leftPins[pin], pinmux.leftVoltage);
			}
			let rightPins = pinmux.rightPins;
			for (let pin = 0, c = rightPins.length; pin <= c; pin++) {
				helper.pinExplorerAddPin(explorer, 59 + pin, rightPins[pin], pinmux.rightVoltage);
			}
//			trace(JSON.stringify(explorer, null, " ") + "\n");
	
			var directions = pinmux.back;
			
			var fixedPins = [
//				{ pin: 1, type: "Ground" },
//				{ pin: 2, type: "Ground" },
				{ pin: 3, type: "Digital", direction: directions[3] },
				{ pin: 4, type: "Digital", direction: directions[4] },
				{ pin: 5, type: "Digital", direction: directions[5] },
				{ pin: 6, type: "Digital", direction: directions[6] },
				{ pin: 7, type: "Digital", direction: directions[7] },
				{ pin: 8, type: "Digital", direction: directions[8] },
				{ pin: 9, type: "Digital", direction: directions[9] },
				{ pin: 10, type: "Digital", direction: directions[10] },
				{ pin: 11, type: "Digital", direction: directions[11] },
				{ pin: 12, type: "Digital", direction: directions[12] },
//				{ pin: 13, type: "Ground" },
//				{ pin: 14, type: "Ground" },
				{ pin: 15, type: "Digital", direction: directions[15] },
				{ pin: 16, type: "Digital", direction: directions[16] },
				{ pin: 17, type: "Digital", direction: directions[17] },
				{ pin: 18, type: "Digital", direction: directions[18] },
				{ pin: 19, type: "Digital", direction: directions[19] },
				{ pin: 20, type: "Digital", direction: directions[20] },
				{ pin: 21, type: "Digital", direction: directions[21] },
				{ pin: 22, type: "Digital", direction: directions[22] },
				{ pin: 23, type: "Digital", direction: directions[23] },
				{ pin: 24, type: "Digital", direction: directions[24] },
//				{ pin: 25, type: "Power", voltage: 3.3 },
//				{ pin: 26, type: "Ground" },
				{ pin: 27, type: "I2CData" },
				{ pin: 28, type: "PWM" },
				{ pin: 29, type: "I2CClock" },
				{ pin: 30, type: "PWM" },
				{ pin: 31, type: "UartTX" },
//				{ pin: 32, type: "Ground" },
				{ pin: 33, type: "UartRX" },
				{ pin: 34, type: "PWM" },
//				{ pin: 35, type: "Ground" },
//				{ pin: 36, type: "Ground" },
//				{ pin: 41, type: "Ground" },
//				{ pin: 42, type: "Ground" },
//				{ pin: 45, type: "Ground" },
//				{ pin: 46, type: "Ground" },
//				{ pin: 49, type: "Power", voltage: 3.3 },
//				{ pin: 50, type: "Power", voltage: 5 },
			];

			for (var i=0, c=fixedPins.length; i<c; i++) {							// add all of the fixed pins
				var aDesc = fixedPins[i];
				helper.pinExplorerAddFixedPin(explorer, aDesc);
			}
		
			Pins.configure(explorer, success => {
				let url = helper.pinsStartSharing(query.ip);
				if (url)
					helper.wsResponse(url)
				else
					helper.wsErrorResponse(500, "Internal Server Error")
			}, error => {
				helper.pinsStopSharing();
				helper.wsErrorResponse(500, "Internal Server Error");
			});
		});
	},
	pinExplorerStop(helper, query) {
		helper.pinsStopSharing();
		helper.wsResponse()
	},
	pinsShare(helper, query) {
		if (query.shared) {
			if (!model.pins)
				model.pins = Pins.share({type: "ws"});
			var url = model.pins[0].instance.url;
			helper.wsResponse(url.replace("*", query.ip));
		}
		else {
			Pins.share();
    		delete model.pins;
			helper.wsResponse();
		}
	},
	clearApps(helper, query) {
		debugger
		let message = new Message("xkpr://shell/settings/clear-apps");
		message.invoke();
		helper.wsResponse();
	},
	clearAppsPrefs(helper, query) {
		let message = new Message("xkpr://shell/settings/clear-apps-prefs");
		debugger
		message.invoke();
		helper.wsResponse();
	},
	clearCookies(helper, query) {
		debugger
		HTTP.Cookies.clear(); 
		K4.log("settings", "Clearing HTTP Cookies");
		helper.wsResponse();
	},
	clearHTTPCache(helper, query) {
		debugger
		HTTP.Cache.clear(); 
		K4.log("settings", "Clearing HTTP Cache");
		helper.wsResponse();
	},
	clearKnownNetworks(helper, query) {
		debugger
		let message = new Message("xkpr://shell/network/known");
		message.method = "DELETE";
		message.invoke();
		helper.wsResponse();
	},
	getNetworkLevel(helper, query) {
		let message = new Message("xkpr://shell/network/status");
		return message.invoke(Message.JSON).then(json => {
			helper.wsResponse(json.signal_level);
		});
	},
	getNetworkList(helper, query) {
		let message = new Message("xkpr://shell/network/status");
		message.invoke(Message.JSON).then(json => {
			helper.wsResponse("scanned" in json ? json.scanned : []);
		});
	},
	getNetworkMAC(helper, query) {
		let message = new Message("xkpr://shell/network/connect/status");
		message.invoke(Message.JSON).then(json => {
			helper.wsResponse(json.address.toUpperCase());
		});
	},
	getStartupApp(helper, query) {
		let message = new Message("xkpr://shell/settings/startup-app");
		return message.invoke(Message.JSON).then(json => {
			helper.wsResponse(json);
		});
	},
	getStartupAppList(helper, query) {
		let message = new Message("xkpr://shell/settings/startup-app-list");
		return message.invoke(Message.JSON).then(json => {
			helper.wsResponse(json);
		});
	},
	getTimezone(helper, query) {
		let message = new Message("xkpr://shell/settings/timezone");
		message.invoke(Message.JSON).then(json => {
			helper.wsResponse(json);
		});
	},
	networkScan(helper, query) {
		let message = new Message(query.active ? "xkpr://shell/network/wifi/scan/start" : "xkpr://shell/network/wifi/scan/stop");
		message.invoke();
		helper.wsResponse();
	},
	setName(helper, query) {
		let message = new Message("xkpr://shell/settings/name");
		message.requestText = JSON.stringify(query.name);
		message.method = "PUT";
		message.invoke();
		helper.wsResponse();
	},
	setTimezone(helper, query) {
		let message = new Message("xkpr://shell/settings/timezone");
		message.requestText = JSON.stringify(query.timezone);
		message.method = "PUT";
		message.invoke();
		helper.wsResponse();
	},
	updateSoftware(helper, query) {
		model.doUpdateSoftware(query.update);
		helper.wsResponse();
	},
	updateSoftwareStatus(helper, query) {
		helper.wsResponse(helper.softwareStatus);
		if (helper.softwareStatus.finished)
			application.invoke(new Message("xkpr://shell/close?id=" + application.id));
	},
	updateSystem(helper, query) {
		model.doUpdateSystem(query.update);
		helper.wsResponse();
	},
	updateSystemStatus(helper, query) {
		helper.wsResponse(helper.systemStatus);
		if (helper.systemStatus.finished)
			application.invoke(new Message("xkpr://shell/close?id=" + application.id));
	}
};

let model = application.behavior = Behavior({
	onLaunch(application) {
		this.softwareStatus = {
			"updating": false,
		};
		this.systemStatus = {
			"updating": false,
		};
		this.data = {};
		application.add(new MainScreen(this.data));

		let connect = getEnvironmentVariable("debugger");
		let url = "http://" + connect.split(":")[0] + ":9999/xsedit?id=" + application.id;
		let message = new Message(url);
		message.invoke(Message.TEXT).then(host => {
			this.xsedit = host;
			this.wsConnect(host);
		});
		setEnvironmentVariable("debugger", "localhost");
		let disconnect = new Message("xkpr://shell/disconnect?terminate=false");
		disconnect.invoke();
	},
	onQuit(application) {
		if (this.pins) {
    		Pins.share();
    		delete this.pins;
    	}
		this.wsDisconnect();
		if (this.reboot)
			K4.reboot();
	},
	// software
	doUpdateSoftware(json) {
		let status = this.softwareStatus;
		let sourceURL = status.sourceURL = json.url;
		let targetURL = status.targetURL = mergeURI(Files.temporaryDirectory, "software");
		status.updating = true;
		status.message = "Downloading Update";
		status.md5 = json.md5;
		Files.deleteFile(targetURL);
		application.invoke(new Message("/downloadSoftware?" + serializeQuery({ sourceURL, targetURL })));
	},
	onSoftwareDownloadError() {
		let status = this.softwareStatus;
		status.message = "Download failed!";
		status.error = true;
	},
	onSoftwareDownloadProgress(offset, size) {
		let status = this.softwareStatus;
		status.offset = offset;
		status.size = size;
	},
	onSoftwareDownloaded() {
		let status = this.softwareStatus;
		status.offset = -1;
		let path;
		if ("toPath" in Files)
			path = Files.toPath(status.targetURL);
		else
			path = status.targetURL.slice(7);
		status.message = "Applying Update";
		status.path = path;
 		application.invoke(new Message("/installSoftware?" + serializeQuery({ path, md5: status.md5 })));
	},
	onSoftwareInstallError() {
		let status = this.softwareStatus;
		status.message = "Update failed!";
		status.error = true;
	},
	onSoftwareInstalled() {
		let status = this.softwareStatus;
		status.finished = true;
		this.reboot = true;
	},
	// system
	doUpdateSystem(json) {
		let status = this.systemStatus;
		let sourceURL = status.sourceURL = json.url;
		let targetURL = status.targetURL = mergeURI(Files.temporaryDirectory, "system");
		status.updating = true;
		status.message = "Downloading Update";
		status.md5 = json.md5;
		Files.deleteFile(targetURL);
		application.invoke(new Message("/downloadSystem?" + serializeQuery({ sourceURL, targetURL })));
	},
	onSystemDownloadError() {
		let status = this.systemStatus;
		status.message = "Download failed!";
		status.error = true;
	},
	onSystemDownloadProgress(offset, size) {
		let status = this.systemStatus;
		status.offset = offset;
		status.size = size;
	},
	onSystemDownloaded() {
		let status = this.systemStatus;
		status.offset = -1;
		let path;
		if ("toPath" in Files)
			path = Files.toPath(status.targetURL);
		else
			path = status.targetURL.slice(7);
		status.message = "Applying Update";
 		application.invoke(new Message("/installSystem?" + serializeQuery({ path, md5: status.md5 })));
	},
	onSystemInstallError() {
		let status = this.systemStatus;
		status.message = "Update failed!";
		status.error = true;
	},
	onSystemInstalled() {
		let status = this.systemStatus;
		status.finished = true;
		this.reboot = true;
	},
	// pins
	pinExplorerAddPin(explorer, pin, type, voltage) {
		switch (type) {
			case PINMUX_DISCONNECTED:
			break;
			case PINMUX_POWER:
				explorer[TYPE_POWER + pin] = {
					pins: { power: { pin, type:TYPE_POWER, voltage } },
					require: TYPE_POWER,
				};					
			break;
			case PINMUX_GROUND:
				explorer[TYPE_GROUND + pin] = {
					pins: { ground: { pin, type:TYPE_GROUND } },
					require: TYPE_GROUND,
				};					
			break;
			case PINMUX_ANALOG:
				explorer[TYPE_ANALOG + pin] = {
					pins: { analog: { pin } },
					require: TYPE_ANALOG,
				};					
			break;
			case PINMUX_DIGITAL_IN:
				explorer[TYPE_DIGITAL + pin] = {
					pins: { digital: { pin, direction:"input" } },
					require: TYPE_DIGITAL,
				};					
			break;
			case PINMUX_DIGITAL_OUT:
				explorer[TYPE_DIGITAL + pin] = {
					pins: { digital: { pin, direction:"output" } },
					require: TYPE_DIGITAL,
				};					
			break;
			case PINMUX_I2C_CLK:
			break;
			case PINMUX_SERIAL_RX:
			break;
			case PINMUX_PWM:
				explorer[TYPE_PWM + pin] = {
					pins: { pwm: { pin } },
					require: TYPE_PWM,
				};					
			break;
		}
	},
	pinExplorerAddFixedPin(explorer, desc) {
		switch (desc.type) {
			case TYPE_POWER:
				explorer[TYPE_POWER + desc.pin] = {
					pins: { power: { pin:desc.pin, type:TYPE_POWER, voltage:desc.voltage } },
					require: TYPE_POWER,
				};					
			break;
			case TYPE_GROUND:
				explorer[TYPE_GROUND + desc.pin] = {
					pins: { ground: { pin:desc.pin, type:TYPE_GROUND } },
					require: TYPE_GROUND,
				};					
			break;
			case TYPE_ANALOG:
				explorer[TYPE_ANALOG + desc.pin] = {
					pins: { analog: { pin:desc.pin, type:TYPE_ANALOG } },
					require: TYPE_ANALOG,
				};					
			break;
			case TYPE_DIGITAL:
				if (desc.direction == "input") {
					explorer[TYPE_DIGITAL + desc.pin] = {
						pins: { digital: { pin:desc.pin, direction:"input" } },
						require: TYPE_DIGITAL,
					};		
				}
				else {
					explorer[TYPE_DIGITAL + desc.pin] = {
						pins: { digital: { pin:desc.pin, direction:"output" } },
						require: TYPE_DIGITAL,
					};		
				}			
			break;
			case TYPE_I2C:
			break;
			case TYPE_SERIAL:
			break;
			case TYPE_PWM:
				explorer[TYPE_PWM + desc.pin] = {
					pins: { pwm: { pin:desc.pin, type:TYPE_PWM } },
					require: TYPE_PWM,
				};					
			break;
		}

	},
	pinsStartSharing(ip) {
 		if (!model.pins)
 			model.pins = Pins.share({type: "ws"});
		var url = model.pins[0].instance.url;
 		return url.replace("*", ip);
	},
	pinsStopSharing() {
		Pins.share();
		Pins.close();
	},
	// websocket
	wsConnect(xsedit) {
		let context = this;
		let url = "ws://" + xsedit;
		
		let helper = this;
		let ws = this.ws = new WebSocket(url);
		ws.onopen = function() {
// 			trace("WSC: onopen\n");
			try {
				this.helper = helper;
				// var uuid = require.weak("uuid");
				let message = new Message(mergeURI(url, "xkpr://shell/description"));
				message.invoke(Message.JSON).then(json => {
					this.send(json.uuid);
				});
			}
			catch (e) {
				debugger;
			}
		}
		ws.onmessage = function(message) {
// 			trace("WSC: onmessage " + message.data + "\n");
			try {
				let json = JSON.parse(message.data);
				let helper = this.helper;
				let handler = ("handler" in json) ? json.handler : undefined;
				if (handler && (handler in handlers))
					handlers[handler](helper, json);
			}
			catch (e) {
				this.helper.wsErrorResponse(500, "Internal Server Error");
			}
		}
		ws.onclose = function() {
// 			trace("WSC: onclose\n");
			application.invoke(new Message("xkpr://shell/close?id=" + application.id));
			this.helper = undefined;
		}
		ws.onerror = function(error) {
			application.invoke(new Message("xkpr://shell/close?id=" + application.id));
// 			trace("WSC: onerror " + error.code + " " + error.reason  + "\n");
		}
	},
	wsDisconnect() {
		if (this.ws) {
// 			trace("WSC: close\n");
			this.ws.close();
			this.ws = undefined;
		}
	},
	wsResponse(content) {
		let json;
		if (content != undefined) {
// 			trace("response: " + JSON.stringify(content) + "\n");
			json = { status:200, content };
		}
		else {
// 			trace("response: -\n");
			json = { status:204 };
		}
		this.ws.send(JSON.stringify(json));
	},
	wsErrorResponse(status, reason) {
		let json = { status, reason };
		this.ws.send(JSON.stringify(json));
	}
});

// settings app

Handler.Bind("/downloadSoftware", class extends Behavior {
	onInvoke(handler, message) {
		var query = parseQuery(message.query);
		handler.download(new Message(query.sourceURL), query.targetURL);
	}
	onProgress(handler, message, offset, size) {
		model.onSoftwareDownloadProgress(offset, size);
	}
	onComplete(handler, message) {
		if (message.error != 0 || message.status != 200)
			model.onSoftwareDownloadError();
		else
			model.onSoftwareDownloaded();
	}
});

Handler.Bind("/installSoftware", class extends Behavior {
	onInvoke(handler, message) {
		Message.notify(new Message("/software/update/start"));
		var query = parseQuery(message.query);
		handler.invoke(new Message("xkpr://k4/doLauncherUpdate?" + serializeQuery({ path:query.path, md5:query.md5 })), Message.TEXT);
	}
	onComplete(handler, message) {
		Message.notify(new Message("/software/update/complete"));
		if (message.error != 0 || message.status != 200)
			model.onSoftwareInstallError();
		else
			model.onSoftwareInstalled();
	}
});

Handler.Bind("/downloadSystem", class extends Behavior {
	onInvoke(handler, message) {
		var query = parseQuery(message.query);
		handler.download(new Message(query.sourceURL), query.targetURL);
	}
	onProgress(handler, message, offset, size) {
		model.onSystemDownloadProgress(offset, size);
	}
	onComplete(handler, message) {
		if (message.error != 0 || message.status != 200)
			model.onSystemDownloadError();
		else
			model.onSystemDownloaded();
	}
});

Handler.Bind("/installSystem", class extends Behavior {
	onInvoke(handler, message) {
		Message.notify(new Message("/software/update/start"));
		var query = parseQuery(message.query);
		handler.invoke(new Message("xkpr://k4/doFirmwareUpdate?" + serializeQuery({ path:query.path, md5:query.md5 })), Message.TEXT);
	}
	onComplete(handler, message) {
		Message.notify(new Message("/software/update/complete"));
		if (message.error != 0 || message.status != 200)
			model.onSystemInstallError();
		else
			model.onSystemInstalled();
	}
});

