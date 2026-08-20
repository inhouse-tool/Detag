# ファイル形式

本アプリが出力するファイルの内容を確認するには,
実例として上げられている
[Detag.Copilot.md](./Detag.Copilot.md) や
[Detag.Gemini.md](./Detag.Gemini.md) を見に行って,
そのページの右上の方にある下記のアイコンをつついて生のファイルイメージを
Copy してからお手許のエディターに Paste したり,<br>
![](../pics/CopyRawFile.png)<br>
同じく下記のアイコンをつついて生のファイルをダウンロードしてからお手許のエディターで開いたり,<br>
![](../pics/DownloadRawFile.png)<br>
とすれば生きた実例が見られます.

> [!IMPORTANT]
> ブラウザー上で右クリックメニューから直接「ページのソースを見る」のを試みても,
`.md` が HTML 変換されたややこしいイメージが見えるばかりです.
「生のファイル」として見てください.


## [`.md`](#readme)

たとえば
[Detag.Copilot.md](./Detag.Copilot.md) を例にとると,
その出だしは下記のように始まっていますが――

<blockquote>
<hr>
<h5>
You said
</h5>
<table style="width: 100%; display: table;"><tr><td>
AI との会話を browser で Save as した .html ファイルがアホみたいにデカいので,<br>
会話のテキストだけ抽出してコンパクトな .htm ファイルにまとめるツールを作った.<br>
このアプリって, ソースコードで公開して問題ないのかな?<br>
あと, このアプリで抽出した会話って, ネットで公開して問題ないのかな?</table>
<h6>
Copilot said
</h6>
<p>
結論だけ先に言うと 
<strong>
あなたのツールは「ソースコード公開OK」
</strong>
 で、
<strong>
抽出した会話の公開は “内容次第でOKにもNGにもなる”
</strong>
 という整理になる。
</p>
<p>
以下、理由を体系的にまとめる。
</p>
</blockquote>

――その `.md` ファイルの生のイメージは, 下記のように始まっています.

<blockquote>

```
<hr>
<h5>
You said
</h5>
<table style="width: 100%; display: table;"><tr><td>
AI との会話を browser で Save as した .html ファイルがアホみたいにデカいので,<br>
会話のテキストだけ抽出してコンパクトな .htm ファイルにまとめるツールを作った.<br>
このアプリって, ソースコードで公開して問題ないのかな?<br>
あと, このアプリで抽出した会話って, ネットで公開して問題ないのかな?</table>
<h6>
Copilot said
</h6>
<p>
結論だけ先に言うと 
<strong>
あなたのツールは「ソースコード公開OK」
</strong>
 で、
<strong>
抽出した会話の公開は “内容次第でOKにもNGにもなる”
</strong>
 という整理になる。
</p>
<p>
以下、理由を体系的にまとめる。
</p>
```
</blockquote>

上記冒頭部分の構成要素は,
本アプリが独自に付け加えているものです.

* 冒頭の `<hr>`:<br>
一つの質疑と次の質疑との間を視覚的に区切るために挟んでいます.

* `<h5>` と `</h5>` で挟まれた `You said`:<br>
Copilot さんや Gemini さんが出力するイメージに元々入っている文言です.
ただ, その出力を直接ブラウザーで眺めている際には隠れていて見えません.
<kbd>Ctrl</kbd>+<kbd>F</kbd> とかして頭出しの検索に便利なので,
本アプリではこの文言が露出するように調整しています.<br>
このような文字列を仕込んではこない Google さんの出力を扱う場合も,
わざわざ同じ文言を挟むようにして取り扱いの共通化を図っています.

* `<table>` と `</table>` で囲まれたユーザーからの質問文:<br>
ユーザー側がどういう質問をぶつけたのかを視覚的に見つけやすくするために囲っています.<br>
オリジナルでは「右側に寄せた独自背景色の矩形の中に納める」というチャットツールのような視覚効果を持たせていますが,
なんかスペースがもったいないのでこのように改めました.

* `<h6>` と `</h6>` で挟まれた `Copilot said`:<br>
Copilot さんが出力するイメージに元々入っている文言です.
ただ, その出力を直接ブラウザーで眺めている際には隠れていて見えません.
<kbd>Ctrl</kbd>+<kbd>F</kbd> とかして頭出しの検索に便利なので,
本アプリではこの文言が露出するように調整しています.<br>
このような文字列を仕込んではこない Google さんの出力を扱う場合も,
下記のような文言をわざわざ挟むようにして共通化を図っています.
そのせいでいずれの AI の出力も統一感のあるデザインに落ち着くので,
この部分を見てどっちの AI との質疑だったのかを見分けたりします.
	<table>
	<tr><td>AI<td>文言
	<tr><td>Copilot<td><tt>Copilot said</tt>
	<tr><td>Gemini web app<td><tt>Gemini said</tt>
	<tr><td>Google AI Overviews<td><tt>Google said</tt>
	</table>

それ以降の部分は,
オリジナルの出力から必要な部分だけを抽出した HTML そのものが入っています.

かといってホントにそのものを入れるとアホみたいなサイズになるので,
下記のように HTML の構成要素を絞って抽出しています.
( 抽出処理については[こちら](./Detag.md)を参照. )

* 不要な HTML タグは剥ぎ取る. <sub>( 本アプリの `Detag` という名前の由来 )</sub>
* 必要な HTML タグでも中身は最小限必要の記述だけに絞る.
* コメントは有無を言わさず全て撤去. <sub>( いずれのオリジナルもやたら無駄なコメントが多い. )</sub>

こうした地道な努力によってリーズナブルなサイズになったテキストをベースに,

1. そのままセーブしたのが `.md` ファイル
1. ブラウザで読めるように最低限のヘッダーとフッターを付けてセーブしたのが `.htm` ファイル

となっています.

`.md` はお好みに応じたエディターやビューアーやブラウザ用プラグインを,
どこからか落としてきてご覧ください.
<br><sup>(
上述のように中身は HTML なので, Windows&reg;11 の
[Notepad](https://apps.microsoft.com/detail/9msmlrh6lzf3?hl=ja-JP&gl=JP) のような「なんちゃって」ではムリです.
)</sup>


## [`.htm`](#readme)

`.htm` ファイルのヘッダーは, 下記のようになっています.

<blockquote>

```
<!DOCTYPE html>
<html>
<head><meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<style>
	body{
		font-family: sans-serif;
		color: #0d1117;	background-color: #e6edf3;
		margin-left: 2%;	margin-right: 2%;
	}
	table{
		border-collapse: collapse;
	}
	th, td{
		border: 1px solid #7f7f7f;
		padding: 8px;
	}

@media ( prefers-color-scheme: dark ){
	body {
		color: #e6edf3;	background-color: #0d1117;
	}
</style>
</head>
<body link="#4088e7" vlink="#4088e7">
```
</blockquote>

このヘッダーでは:

* UTF-8 の文字コードが通るようにしてある. <sub>( 特に日本語を選択しているわけではない )</sub>
* ダークモードの表示に対応させている.
* デフォルトだとリンクを示す文字の青色が濃すぎて見づらいので, 今風の薄味にしてある.

といった程度のことしかしていません.
<br><sup>(
このへんも凝るとキリがないですよね. 凝りたい方はソースをいじってどうぞ.
)</sup>

一方のフッターは下記のようになっています.
<blockquote>

```
</body>
</html>
```
</blockquote>

つまり HTML として閉じているだけです.

HTML ならファイルにしまうときには拡張子は `.html` にするのが普通だろう。との想定で,
本アプリは入力のファイルを `.html` ということにしています.
で, その `.html` を上書きしてしまわないように出力のファイルを `.htm` としているわけです.

もし, オリジナルを `.htm` としてセーブしてあった場合は,
お手数ですが本アプリに食わせる前に `.html` にリネームしておいてください.
いや, `.htm` をリネームする前に `_file` フォルダを先にリネームして,
`.htm` との縁を切ってください. それから `_files` フォルダを削除するんです.
<br><sup>(
お手数ですが,
これも[ファイルの接続](https://learn.microsoft.com/ja-jp/windows/win32/shell/manage#connected-files)とかいう,
アホみたいな仕様による連動のせいでややこしいことになるのを避けるためなんです.
)</sup>

