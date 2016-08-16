import {
	model,
} from "shell/main";

import { 
	DeviceConfig, 
	updateCredentials 
} from "features/devices/devices";

export default class nanopim1 extends DeviceConfig {
	constructor(devices, discovery) {
		super(devices, discovery);
		if (!("firmware" in this.description))
			this.description.firmware = getEnvironmentVariable("OS") + " " + getEnvironmentVariable("OSVersion");
	}
	get helperID() {
		return "xsedit.nanopim1.kinoma.marvell.com";
	}
	get helperProject() {
		return {
			id: this.helperID,
			url: mergeURI(this.constructor.url, "./helper/"),
			query: {
				id: this.helperID,
				title: "Kinoma Code Helper",
			}
		};
	}
	getUpdateInfo() {
// 		var software = new Message("https://auth.developer.cloud.kinoma.com/kinoma-device-update?target=CREATE_SHELL_RELEASE");
// 		software.setRequestHeader("Accept", "application/json");
// 		software.setRequestCertificate(updateCredentials);
// 		var system = new Message("https://auth.developer.cloud.kinoma.com/kinoma-device-update?target=CREATE_FIRMWARE_RELEASE");
// 		system.setRequestHeader("Accept", "application/json");
// 		system.setRequestCertificate(updateCredentials);
// 		return Promise.all([
// 			software.invoke(Message.JSON).then(json => { return json }),
// 			system.invoke(Message.JSON).then(json => { return json }),
// 		]);
	}
	isSimulator() {
		return this.systemVersion.indexOf(getEnvironmentVariable("OS")) == 0;
	}
	// HELPER
	clearApps() {
		return this.wsRequest({
			handler: "clearApps",
		});
	}
	clearAppsPrefs() {
		return this.wsRequest({
			handler: "clearAppsPrefs",
		});
	}
	clearCookies() {
		return this.wsRequest({
			handler: "clearCookies",
		});
	}
	clearHTTPCache() {
		return this.wsRequest({
			handler: "clearHTTPCache",
		});
	}
	clearKnownNetworks() {
		return this.wsRequest({
			handler: "clearKnownNetworks",
		});
	}
	getStartupApp() {
		return this.wsRequest({
			handler: "getStartupApp",
		});
	}
	getStartupAppList() {
		return this.wsRequest({
			handler: "getStartupAppList",
		});
	}
	networkScan(active) {
		return this.wsRequest({
			handler: "networkScan",
			active,
		});
	}
	pinExplorerStart(container) {
		return this.wsRequest({
			handler: "pinExplorerStart",
			ip: this.currentIP,
		});
	}
	pinExplorerStop(container) {
		return this.wsRequest({
			handler: "pinExplorerStop",
		});
	}
	pinsShare(shared) {
		return this.wsRequest({
			handler: "pinsShare",
			shared,
			ip: this.currentIP,
		});
	}
	// STUDIO
    getNetworkSSID() {
        if (model.SSID)
            return Promise.resolve(model.SSID);
        else {
            let message = this.newStudioMessage("/network/status");
            return message.invoke(Message.JSON).then(status => {
                if (status && "ssid" in status)
                    model.SSID = status.ssid;
                return Promise.resolve(model.SSID);
            });
        }
    }
	networkConfigure(config) {
		this.wsClose();
		let delay = new Message("/network/configure?" + serializeQuery({uuid:this.uuid}));
		delay.requestText = JSON.stringify(config);
		return delay.invoke().then(message => {
			model.devicesFeature.onDeviceDown(this);
		});
	}
	newPingMessage() {
		return this.newStudioMessage("/info");
	}
	// UPDATE
	updateSoftware() {
		return this.wsRequest({
			handler: "updateSoftware",
			update: this.update[0],
		});
	}
	updateSoftwareStatus() {
		return this.wsRequest({
			handler: "updateSoftwareStatus",
		});
	}
	updateSystem() {
		return this.wsRequest({
			handler: "updateSystem",
			update: this.update[1],
		});
	}
	updateSystemStatus() {
		return this.wsRequest({
			handler: "updateSystemStatus",
		});
	}
}

Handler.Bind("/network/configure", class extends Behavior {
	onInvoke(handler, message) {
		handler.wait(3000);
	}
	onComplete(handler, message) {
		let query = parseQuery(handler.message.query);
		let device = model.devicesFeature.findDeviceByUUID(query.uuid);
		let config = JSON.parse(handler.message.requestText);
		let configure = device.newStudioMessage("/network/configure", config);
		configure.invoke();
	}
});

nanopim1.iconSkin = new Skin({ texture:new Texture("./icon.png", 1), x:0, y:0, width:60, height:60 });
nanopim1.id = "com.marvell.kinoma.launcher.nanopim1";
nanopim1.product = "NanoPi M1";
nanopim1.tag = "nanopim1";
nanopim1.templateTag = "createSample";
nanopim1.url = this.uri;

import {
	BLLExplorerTile,
	BLLExplorerView 
} from "features/devices/apps/bllexplorer/bllexplorer";

import {
	FrontPinsTile,
	FrontPinsView 
} from "features/devices/apps/frontpins/frontpins";

import {
	PinExplorerTile,
	PinExplorerView 
} from "features/devices/apps/pinexplorer/pinexplorer";

import {
	SettingsTile,
	SettingsView 
} from "features/devices/apps/settings/settings";

import {
	WifiTest,
	WifiTile,
	WifiView 
} from "features/devices/apps/wifi/wifi";

nanopim1.apps = {
	wifi: {
		Test: WifiTest,
		Tile: WifiTile,
		View: WifiView
	},
	settings: {
		Tile: SettingsTile,
		View: SettingsView
	},
	pinexplorer: {
		Tile: PinExplorerTile,
		View: PinExplorerView
	},
//	bllexplorer: {
//		Tile: BLLExplorerTile,
//		View: BLLExplorerView
//	},
}

nanopim1.preferences = {
	discoveryFlag: true,
	serialFlag: false,
}

