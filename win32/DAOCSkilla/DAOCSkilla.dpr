program DAOCSkilla;

uses
  Forms,
  Unit1 in 'Unit1.pas' {frmMain},
  bpf in '..\Common\PacketSniff\bpf.pas',
  DAOCConnection in '..\Common\PacketSniff\DAOCConnection.pas',
  FrameFns in '..\Common\PacketSniff\FrameFns.pas',
  DAOCInventory in '..\Common\DAOCInfo\daocinventory.pas',
  DAOCObjs in '..\Common\DAOCInfo\DAOCObjs.pas',
  DAOCPlayerAttributes in '..\Common\DAOCInfo\DAOCPlayerAttributes.pas',
  DAOCRegion in '..\Common\DAOCInfo\DAOCRegion.pas',
  VendorItems in '..\Common\DAOCInfo\VendorItems.pas',
  MapNavigator in '..\Common\MapNavigator.pas',
  StringParseHlprs in '..\Common\StringParseHlprs.pas',
  ChatParse in '..\Common\ChatParse\ChatParse.pas',
  LinedFileStream in '..\Common\LinedFileStream.pas',
  DAOCSkilla_TLB in 'DAOCSkilla_TLB.pas',
  MPKFile in '..\Common\MPKFile.pas',
  CSVLineParser in '..\Common\CSVLineParser.pas',
  Recipes in '..\Common\DAOCInfo\Recipes.pas',
  DAOCControl in '..\Common\DAOCAutomation\DAOCControl.pas',
  sndkey32 in '..\Common\DAOCAutomation\SNDKEY32.pas',
  DAOCWindows in '..\Common\DAOCInfo\DAOCWindows.pas',
  PowerSkill in '..\Common\DAOCAutomation\PowerSkill.pas',
  PowerSkillSetup in '..\Common\DAOCAutomation\PowerSkillSetup.pas' {frmPowerskill},
  ShowMapNodes in '..\Common\ShowMapNodes.pas' {frmShowMapNodes},
  MacroTradeSkill in '..\Common\DAOCAutomation\MacroTradeSkill.pas' {frmMacroTradeSkills},
  AFKMessage in '..\Common\DAOCAutomation\AFKMessage.pas' {frmAFK},
  AXScript in '..\Common\DAOCAutomation\AXScript.pas',
  ScriptSiteImpl in '..\Common\DAOCAutomation\ScriptSiteImpl.pas',
  SpellcraftHelp in '..\Common\DAOCAutomation\SpellcraftHelp.pas' {frmSpellcraftHelp},
  DDSImage in 'DDSImage.pas',
  VCLMemStrms in '..\Common\VCLMemStrms.pas',
  DAOCPackets in '..\Common\PacketSniff\DAOCPackets.pas',
  DAOCClasses in '..\Common\DAOCInfo\DAOCClasses.pas',
  DAOCConSystem in '..\Common\DAOCInfo\DAOCConSystem.pas',
  DebugAndTracing in 'DebugAndTracing.pas' {frmDebugging},
  GlobalTickCounter in '..\Common\GlobalTickCounter.pas',
  QuickSinCos in '..\Common\QuickSinCos.pas',
  Macroing in 'Macroing.pas' {frmMacroing},
  Intersections in '..\Common\Intersections.pas',
  BackgroundHTTP in 'BackgroundHTTP.pas',
  ConnectionConfig in 'ConnectionConfig.pas' {frmConnectionConfig},
  RemoteAdmin in '..\Common\DAOCAutomation\RemoteAdmin.pas' {dmdRemoteAdmin: TDataModule},
  zlib2 in '..\Components\ZLib\zlib2.pas',
  QuickLaunchChars in 'QuickLaunchChars.pas';

{$R *.TLB}

{$R *.res}

begin
  Application.Initialize;
  Application.Title := 'DAOCSkilla for Dark Age of Camelot';
  Application.CreateForm(TfrmMain, frmMain);
  Application.CreateForm(TfrmPowerskill, frmPowerskill);
  Application.CreateForm(TfrmShowMapNodes, frmShowMapNodes);
  Application.CreateForm(TfrmMacroTradeSkills, frmMacroTradeSkills);
  Application.CreateForm(TfrmAFK, frmAFK);
  Application.CreateForm(TfrmSpellcraftHelp, frmSpellcraftHelp);
  Application.CreateForm(TfrmDebugging, frmDebugging);
  Application.CreateForm(TfrmMacroing, frmMacroing);
  Application.CreateForm(TfrmConnectionConfig, frmConnectionConfig);
  Application.CreateForm(TdmdRemoteAdmin, dmdRemoteAdmin);
  CreateOptionalForms;
  Application.Run;
end.
