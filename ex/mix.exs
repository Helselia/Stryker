defmodule Toku.MixProject do
  use Mix.Project

  def project do
    [
      app: :toku,
      version: "0.1.0",
      elixir: "~> 1.11",
      start_permanent: Mix.env() == :prod,
      deps: deps()
    ]
  end

  # Run "mix help compile.app" to learn about applications.
  def application do
    [
      mod: {Toku, []},
      extra_applications: [:logger]
    ]
  end

  # Run "mix help deps" to learn about dependencies.
  defp deps do
    [
      {:cowboy, "~> 2.8"},
      {:plug, "~> 1.11"},
      {:plug_cowboy, "~> 2.0"},
      {:poison, "~> 4.0.1"}
    ]
  end
end
