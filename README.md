<p align="center">
  <img src="https://github.com/TheQmaks/JavaInjector/blob/master/logo.png">
  
  <a href="https://github.com/TheQmaks/JavaInjector/releases">
    <img src="https://img.shields.io/github/v/release/TheQmaks/JavaInjector?style=for-the-badge"><!--
    --><img src="https://img.shields.io/github/downloads/TheQmaks/JavaInjector/total?style=for-the-badge">
  </a>
</p>

This tool inject your jar file and allocates object of class, which name must be written in zip comment.
Entry point of your main-class will be constructor.
After building you classes, you should put them into archive using WinRAR of other archivator which can change archive comment.
You should write main class in comment as single line.
Example:
Put class with name "Main" inside package "test", constructor will be your entry-point, e.g
"public Main() { Your awesome code }", so you should set comment in your archive with classes to "test.Main" without quotes. 
