'use strict';

import * as vscode from 'vscode';
import {activateMyDebug} from './my-debug';

class DAServerDescriptorFactory implements vscode.DebugAdapterDescriptorFactory
{
  createDebugAdapterDescriptor(session: vscode.DebugSession, executable: vscode.DebugAdapterExecutable | undefined): vscode.ProviderResult<vscode.DebugAdapterDescriptor> {
    return new vscode.DebugAdapterServer(7766, "127.0.0.1");
  }

  dispose(): void
  {}

}


export function activate(context: vscode.ExtensionContext)
{
  activateMyDebug(context, new DAServerDescriptorFactory());
}

export function deactivate()
{
}


