/// <reference path="../types/vizzu.d.ts" />

import Vizzu from '../vizzu.js'
import { NotInitializedError } from '../errors.js'

export class Rendering implements Plugins.Plugin {
  _vizzu?: Vizzu

  meta = { name: 'rendering' }

  register(vizzu: Vizzu) {
    this._vizzu = vizzu
  }

  enable(enabled: boolean) {
    if (!this._vizzu || !this._vizzu._chart) throw new NotInitializedError()
    this._vizzu._chart._render._enabled = enabled
  }
}
