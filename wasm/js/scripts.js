function init() {
	initQTwasm('.', 'server_main', '#qtrootDiv', 'img/qtlogo.svg');
	checkModuleLoad = setInterval(() => {
		if (qtLoader.module()) {
			resizeSplitX();
			clearInterval(checkModuleLoad);
		}

		if (typeof counter === 'undefined') {
			counter = 0;
		}

		counter++;
		if (counter > 60) {
			clearInterval(checkModuleLoad);
		}
	}, 1000);

}

function LoadFile(readme) {
	axios.get(readme).then(response => {
		document.querySelector('#md').mdContent = response.data;
	}).catch(error => {
		console.log(error);
	});
}

function resizeSplitX(event) {
	const canvas = document.querySelector('#screen');
	qtLoader.resizeCanvasElement(canvas);
}
