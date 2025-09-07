'use strict';

import * as vscode from 'vscode';


export function activate(context: vscode.ExtensionContext)
{
  console.log('ACTIVATING FIRST !')
}

export function deactivate()
{
}


class MyDebugAdapterDescriptorFactory implements vscode.DebugAdapterDescriptorFactory
{

}