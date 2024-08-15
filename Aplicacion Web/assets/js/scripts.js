// En scripts.js
function exportTxt() {
    if (hasDisconnectedBlocks()) {
        alert("Hay bloques desconectados. Por favor, conecte todos los bloques antes de exportar.");
    } else {
        var code = Blockly.JavaScript.workspaceToCode(window.workspace);
        console.log("Generated code:", code);  // Esto te ayudará a ver qué se está generando
        if (code === "") {
            console.log("No blocks or missing block definitions.");  // Para depuración
        } else {
            download("program.txt", code);
        }
    }
}

function hasDisconnectedBlocks() {
    var blocks = window.workspace.getAllBlocks();
    for (var i = 0; i < blocks.length; i++) {
        if (!blocks[i].getPreviousBlock() && !blocks[i].getNextBlock() && !blocks[i].outputConnection && !blocks[i].parentBlock_) {
            // Block is not connected to any other block
            return true;
        }
    }
    return false;
}

function download(filename, text) {
    var element = document.createElement('a');
    element.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(text));
    element.setAttribute('download', filename);

    element.style.display = 'none';
    document.body.appendChild(element);

    element.click();

    document.body.removeChild(element);
}

document.addEventListener('DOMContentLoaded', function() {
    window.workspace = Blockly.inject('blocklyDiv', {
        toolbox: document.getElementById('toolbox')
    });
});
