// See the Electron documentation for details on how to use preload scripts:
// https://www.electronjs.org/docs/latest/tutorial/process-model#preload-scripts

import { contextBridge, ipcRenderer } from "electron";

declare global {
    interface Window {
        ipc: {
            handleHttp: (cb: (path: string) => void) => void;
        };
    }
}

contextBridge.exposeInMainWorld('ipc', {
    handleHttp: (cb: (path: string) => void) => ipcRenderer.on("http", (ev, args) => {
        cb(args);
    }),
});
