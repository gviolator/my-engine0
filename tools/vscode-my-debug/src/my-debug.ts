'use strict';

import * as vscode from 'vscode';


export function activateMyDebug(context: vscode.ExtensionContext, factory: vscode.DebugAdapterDescriptorFactory) {

  context.subscriptions.push(vscode.debug.registerDebugAdapterDescriptorFactory('my-embedded', factory));

  if ('dispose' in factory) {
    const disposable = (factory as {dispose(): void});
    context.subscriptions.push(disposable);
  }

}