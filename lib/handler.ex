defmodule Toku.Handler do
  @type encoding :: String.t()
  @type compression :: String.t()
  @type options :: %{
    supported_encodings: [encoding],
    supported_compression: [compression]
  }
  @type reason :: atom | tuple

  @callback toku_init(:ranch.transport(), keyword) :: {:ok, options}
  @callback toku_request(any, String.t()) :: any
  @callback toku_push(any, String.t()) :: :ok
  @callback toku_terminate(reason) :: :ok

  @optional_callbacks [toku_push: 2]
end
