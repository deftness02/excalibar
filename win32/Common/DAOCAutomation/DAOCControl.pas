unit DAOCControl;

interface

uses
  SysUtils, Windows, Messages, Classes, SndKey32, DAOCConnection, DAOCObjs,
  MapNavigator, ExtCtrls, DAOCWindows, MMSystem, ComObj, ActiveX, AxCtrls,
  DAOCSkilla_TLB, Recipes, MPKFile;

{$WARN SYMBOL_PLATFORM OFF}

type
  TAutomationMode = (amNone, amCommission);
  TCommissionAutomationState = (casUnknown, casNoCommission, casGotCommission,
    casGatherMetal, casGatherWood, casGatherLeather, casReadyToBuild, casHaveItem);

  TDAOCControl = class(TDAOCConnection, IConnectionPointContainer,
    IDAOCControl)
  private
    { note: FEvents maintains a *single* event sink. For access to more
      than one event sink, use FConnectionPoint.SinkList, and iterate
      through the list of sinks. }
    FAxEvents:  IDAOCControlEvents;
    FConnectionPoints:  TConnectionPoints;
    FConnectionPoint:   TConnectionPoint;

    FMainHWND: HWND;
    FDAOCHWND: HWND;
    FArrowLeft: boolean;
    FArrowUp: boolean;
    FArrowDown: boolean;
    FArrowRight: boolean;

    FLastPlayerPos: TDAOCMovingObject;
    FSamePlayerPosCount:  integer;
    FMapNodes:    TMapNodeList;
    FTurnRateCalLeft: integer;
    FTurnRate:    integer; // (ticks * 100) per degree

    FCurrentPath:     TMapNodeList;
    FCurrentPathIdx:  integer;
    FDestGotoNode:  TMapNode;
    FDestGotoNodeFreeable: boolean;
    FGotoDistTolerance: integer;
    FOnArriveAtGotoDest: TNotifyEvent;
    FDAOCPath: string;
    FWindowManager:   TDAOCWindowManager;
    FForceStationaryTurnDegrees: integer;
    FAutomationMode:  TAutomationMode;
    FCommissionAutomationState: TCommissionAutomationState;
    FOnPathChanged: TNotifyEvent;
    FOnStopAllActions: TNotifyEvent;
    FTradeSkillTargetQuality: integer;
    FTradeSkillTargetSound: string;
    FTradeSkillProgression: string;
    FTradeSkillProgressionIdx:  integer;
    FTradeSkillStopIfFull: boolean;
    FAFKMessage: string;
    FTradeSkillOddsloadCount: integer;
    FTradeSkillOddsloadKey: string;
    FTradeSkillOddsloadPct: integer;

    FTradeRecipes:  TUniversalRecipeCollection;
    
    function BearingToDest: integer;
    procedure SetArrowDown(const Value: boolean);
    procedure SetArrowLeft(const Value: boolean);
    procedure SetArrowRight(const Value: boolean);
    procedure SetArrowUp(const Value: boolean);
    procedure TurnRateNeeded;
    procedure SetDAOCPath(const Value: string);
//    procedure TradeskillSuccessCallback(Sender: TObject; AParm: LPARAM);
    procedure SetAutomationMode(const Value: TAutomationMode);
    function GetTradeMasterName : string;
    function CheckHaveTradeCommissionItem : boolean;
    function CheckNeedMaterials : boolean;
    function StepCurrentPath: boolean;
    procedure SetTradeSkillProgression(const Value: string);
    function GetClassTypeInfo: ITypeInfo;
    function GetTradeRecipes: TUniversalRecipeCollection;
  protected
    procedure DoTurntoDest(AMaxTurnTime: integer);
    function PlayerToHeadingDelta(AHead: integer) : integer;
    procedure DoGotoDest;
    procedure ClearGotoDest;
    procedure ClearCurrentPath;
    procedure StopGotoDest;
    procedure AllKeysUp;
    procedure SendVKDown(vk: byte; bDown: boolean);
    procedure LeftClickEvent(Sender: TObject; X, Y: integer);
    procedure RightClickEvent(Sender: TObject; X, Y: integer);
    procedure SendKeysEvent(Sender: TObject; const AKeys: string);
    procedure SendVKUpEvent(Sender: TObject; vk: byte);
    procedure SendVKDownEvent(Sender: TObject; vk: byte);
    function ContinueCurrentPath : boolean;
    function WhithinToleranceOfDest : boolean;
    procedure AdjustWindowCoords(var X, Y: integer);
    function NextNodeInPath : TMapNode;
    function NextNodeInPathIsCoastTurnable : boolean;
    procedure ContinueTurnRateCal;
    procedure SetGotoDest(ANode: TMapNode);
    function StuckGoingToDest : boolean;
    function FindDAOCWindow : HWND;
    procedure SendOneKeyNoFocus(c: char);
    procedure UpdateDAOCWindowHnd;
    function LastItemOfTradeskillProgressionIsNext : boolean;
    function LoadOddsStreakContinue : boolean;
    function LoadOddsPctContinue : boolean;

    procedure LoadMasterVendorListForRegion(ARegionID: integer);
    procedure SaveMasterVendorListForRegion(ARegionID: integer);

    procedure DoOnChatSendIncoming(const AWho, AMessage: string); override;
    procedure DoOnPopupMessage(const AMessage: string); override;
    procedure DoOnTradeCommissionAssigned; override;
    procedure DoOnTradeskillTaskCompleted; override;
    procedure DoOnTradeskillSuccess(AQuality: integer); override;
    procedure DoOnTradeskillFailure; override;
    procedure DoOnTradeskillCapped; override;
    procedure DoOnPlayerPosUpdate; override;
    procedure DoOnCharacterLogin; override;
    procedure DoOnConnect; override;
    procedure DoOnDisconnect; override;
    procedure DoOnArriveAtGotoDest;
    procedure DoOnPathChanged;
    procedure DoSetRegionID(ARegion: integer); override;
    procedure DoOnSelectedObjectChanged(AObject: TDAOCObject); override;
    procedure DoOnCombatStyleSuccess(AStyle: string); override;
    procedure DoOnCombatStyleFailure; override;

      { IDAOCControl overrides }
    procedure IDAOCControl.SendKeys = SendKeysW;
    procedure SendKeysW(const s: WideString); safecall;
    procedure IDAOCControl.Log = DAOCCLog;
    procedure DAOCCLog(const AMessage: WideString); safecall;
      { IConnectionPoints }
    property ConnectionPoints: TConnectionPoints read FConnectionPoints
      implements IConnectionPointContainer;
    procedure EventSinkChanged(const EventSink: IUnknown); override;
  public
    constructor Create; override;
    destructor Destroy; override;

    procedure Initialize; override;
    procedure Clear; override;

    procedure DoVKDown(vk: byte);
    procedure DoVKUp(vk: byte);
    procedure DoSendKeys(const s: string);
    procedure StopAllActions; safecall;
    procedure Jump; safecall;
    procedure QuitDAOC; safecall;
    procedure SetQuickbarPage(APage: integer); safecall;
    procedure LeftClick(X, Y: integer); safecall;
    procedure RightClick(X, Y: Integer); safecall;
    procedure IDAOCControl.Sleep = DAOCCSleep;
    procedure DAOCCSleep(dwTime: integer); safecall;
    procedure SelectGroupMember(AIndex: integer); safecall;
    procedure Stick; safecall;
    procedure Face; safecall;
    procedure Follow; safecall;

    procedure IDAOCControl.ChatSend = ChatSendW;
    procedure ChatSendW(const bsWho: WideString; const bsMessage: WideString); safecall;
    procedure IDAOCControl.ChatSay = ChatSayW;
    procedure ChatSayW(const bsMessage: WideString); safecall;
    procedure IDAOCControl.ChatGuild = ChatGuildW;
    procedure ChatGuildW(const bsMessage: WideString); safecall;
    procedure IDAOCControl.ChatGroup = ChatGroupW;
    procedure ChatGroupW(const bsMessage: WideString); safecall;
    procedure IDAOCControl.ChatAlliance = ChatAllianceW;
    procedure ChatAllianceW(const bsMessage: WideString); safecall;
    procedure IDAOCControl.ChatChat = ChatChatW;
    procedure ChatChatW(const bsMessage: WideString); safecall;

    procedure ChatSend(const AWho, AMessage: string);
    procedure ChatSay(const AMessage: string);
    procedure ChatGuild(const AMessage: string);
    procedure ChatGroup(const AMessage: string);
    procedure ChatAlliance(const AMessage: string);
    procedure ChatChat(const AMessage: string);
    procedure SetPlayerHeading(AHead, AMaxTurnTime: integer);
    procedure TurnRateRecalibrate;
    procedure GotoXY(X, Y: DWORD);
    procedure GotoNode(ANode: TMapNode);
    procedure FaceNode(ANode: TMapNode; AMaxTurnTime: integer);
    function PathToNode(ANode: TMapNode) : boolean;
    function PathToNodeName(const ANodeName: string) : boolean;
    procedure NodeAddAtPlayerPos(const AName: string);
    function NodeClosestToPlayerPos : TMapNode;
    function ConnectedNodeClosestToPlayerPos : TMapNode;
    procedure CloseDialog;
    procedure FocusDAOCWindow;
    procedure TradeskillContinueProgression;

      { read-only props }
    property TurnRate: integer read FTurnRate;
    property MapNodes: TMapNodeList read FMapNodes;
    property CurrentPath: TMapNodeList read FCurrentPath;
    property WindowManager: TDAOCWindowManager read FWindowManager;
    property ClassTypeInfo: ITypeInfo read GetClassTypeInfo;
    property TradeRecipes: TUniversalRecipeCollection read GetTradeRecipes;
      { internal props }
    property MainHWND: HWND read FMainHWND write FMainHWND;
      { Configuration tweaks }
    property AFKMessage: string read FAFKMessage write FAFKMessage;
    property DAOCPath: string read FDAOCPath write SetDAOCPath;
    property GotoDistTolerance: integer read FGotoDistTolerance write FGotoDistTolerance;
    property ForceStationaryTurnDegrees: integer read FForceStationaryTurnDegrees
      write FForceStationaryTurnDegrees;
    property TradeSkillProgression: string read FTradeSkillProgression write SetTradeSkillProgression;
    property TradeSkillStopIfFull: boolean read FTradeSkillStopIfFull write FTradeSkillStopIfFull;
    property TradeSkillTargetQuality: integer read FTradeSkillTargetQuality write FTradeSkillTargetQuality;
    property TradeSkillTargetSound: string read FTradeSkillTargetSound write FTradeSkillTargetSound;
    property TradeSkillOddsloadKey: string read FTradeSkillOddsloadKey write FTradeSkillOddsloadKey;
    property TradeSkillOddsloadCount: integer read FTradeSkillOddsloadCount write FTradeSkillOddsloadCount;
    property TradeSkillOddsloadPct: integer read FTradeSkillOddsloadPct write FTradeSkillOddsloadPct;
      { DAOC Props }
    property ArrowUp: boolean read FArrowUp write SetArrowUp;
    property ArrowDown: boolean read FArrowDown write SetArrowDown;
    property ArrowLeft: boolean read FArrowLeft write SetArrowLeft;
    property ArrowRight: boolean read FArrowRight write SetArrowRight;
    property AutomationMode: TAutomationMode read FAutomationMode write SetAutomationMode;
      { Events }
    property OnArriveAtGotoDest: TNotifyEvent read FOnArriveAtGotoDest write FOnArriveAtGotoDest;
    property OnPathChanged: TNotifyEvent read FOnPathChanged write FOnPathChanged;
    property OnStopAllActions: TNotifyEvent read FOnStopAllActions write FOnStopAllActions; 
  end;

implementation

uses
  ComServ, ChatParse;

const
  TURNRATE_CAL_COUNT = 5;

function W2K_SetForegroundWindow(hWndToForce : HWND) : Boolean;
var
   hCurrWnd:  HWND;
   dwMyTID:   DWORD;
   dwCurrTID: DWORD;
begin
   hCurrWnd := GetForegroundWindow;
   dwMyTID   := GetCurrentThreadId;
   dwCurrTID := GetWindowThreadProcessId(hCurrWnd, nil);

   AttachThreadInput(dwMyTID, dwCurrTID, True);
    { Now we look like the foreground process, we can set the foreground window. }
   Result := SetForegroundWindow (hWndToForce);
   AttachThreadInput (dwMyTID, dwCurrTID, False);
end;

function AbsI(i: Integer) : integer;
begin
  if I < 0 then
    Result := -I
  else
    Result := I;
end;

{ TDAOCControl }

destructor TDAOCControl.Destroy;
begin
  FTradeRecipes.Free;
  FWindowManager.Free;
  StopAllActions;
  FLastPlayerPos.Free;
  FMapNodes.Free;
  inherited Destroy;
end;

procedure TDAOCControl.DoOnPlayerPosUpdate;
begin
    { if we're calibrating the turn rate }
  if FTurnRateCalLeft > 0 then
    ContinueTurnRateCal;

  if FLastPlayerPos.SameLoc(FLocalPlayer) then
    inc(FSamePlayerPosCount)
  else
    FSamePlayerPosCount := 0;

  FLastPlayerPos.Assign(FLocalPlayer);

  if Assigned(FDestGotoNode) then
    DoGotoDest;

  inherited;

  if Assigned(FAxEvents) then
    FAxEvents.OnPlayerPosUpdate;
end;

procedure TDAOCControl.SendKeysW(const S: WideString);
begin
  DoSendKeys(s);
end;

procedure TDAOCControl.DoSendKeys(const S: string);
var
  I:      integer;
  hFore:  HWND;
begin
  if S = '' then
    exit;

  UpdateDAOCWindowHnd;

  hFore := GetForegroundWindow;
  if hFore = MainHWND then
    exit;

  if hFore = FDAOCHWND then
    SndKey32.SendKeys(PChar(S), false)
  else
    for I := 1 to Length(S) do begin
      SendOneKeyNoFocus(S[I]);
      sleep(10);
    end;
end;

procedure TDAOCControl.SendVKDown(vk: byte; bDown: boolean);
var
  Flags:    DWORD;
  ScanCode: byte;
begin
  if bDown then
    Flags := 0
  else
    Flags := KEYEVENTF_KEYUP;
  ScanCode := Lo(MapVirtualKey(vk, 0));
  keybd_event(vk, ScanCode, Flags, 0);
end;

procedure TDAOCControl.AllKeysUp;
begin
  SetArrowDown(false);
  SetArrowUp(false);
  SetArrowLeft(false);
  SetArrowRight(false);
end;

procedure TDAOCControl.SetArrowDown(const Value: boolean);
begin
  if FArrowDown = Value then
    exit;
  FArrowDown := Value;
  SendVKDown(VK_DOWN, FArrowDown);
end;

procedure TDAOCControl.SetArrowLeft(const Value: boolean);
begin
  if FArrowLeft = Value then
    exit;
  FArrowLeft := Value;
  SendVKDown(VK_LEFT, FArrowLeft);
end;

procedure TDAOCControl.SetArrowRight(const Value: boolean);
begin
  if FArrowRight = Value then
    exit;
  FArrowRight := Value;
  SendVKDown(VK_RIGHT, FArrowRight);
end;

procedure TDAOCControl.SetArrowUp(const Value: boolean);
begin
  if FArrowUp = Value then
    exit;
  FArrowUp := Value;
  SendVKDown(VK_UP, FArrowUp);
end;

procedure TDAOCControl.StopAllActions;
begin
  AllKeysUp;
  ClearCurrentPath;
  StopGotoDest;

  if Assigned(FOnStopAllActions) then
    FOnStopAllActions(Self);
end;

constructor TDAOCControl.Create;
begin
  inherited Create;

  FTurnRate := 700;
  FForceStationaryTurnDegrees := 50;

  FLastPlayerPos := TDAOCMovingObject.Create;
  FMapNodes := TMapNodeList.Create;
  FWindowManager := TDAOCWindowManager.Create;
  FWindowManager.OnNeedLeftClick := LeftClickEvent;
  FWindowManager.OnNeedRightClick := RightClickEvent;
  FWindowManager.OnNeedSendKeys := SendKeysEvent;
  FWindowManager.OnNeedVKUp := SendVKUpEvent;
  FWindowManager.OnNeedVKDown := SendVKDownEvent;
end;

procedure TDAOCControl.QuitDAOC;
begin
  DoSendKeys('/q[cr]');
end;

procedure TDAOCControl.SetQuickbarPage(APage: integer);
begin
  if not (APage in [1..10]) then
    exit;

//  SendVKDown(VK_SHIFT, true);
//  SendKeys(PChar(IntToStr(APage)), true);
//  Sleep(500);
//  SendVKDown(VK_SHIFT, false);
  DoSendKeys('/qbar ' + IntToStr(APage) + '[cr]');
end;

procedure TDAOCControl.SetPlayerHeading(AHead, AMaxTurnTime: integer);
var
  iDegToTurn:   integer;
  iExpectedTurnTime:  integer;
begin
  TurnRateNeeded;

  iDegToTurn := PlayerToHeadingDelta(AHead);
  iExpectedTurnTime := AbsI((iDegToTurn * FTurnRate) div 100);

    { if the turn is less than 10ms forget it, because we'll more than likely oversteer }
  if iExpectedTurnTime < 10 then
    exit;

  if iDegToTurn >= 5 then
    Log(Format('Turn %d degrees, %d ms/deg, sleep for %d ms',
      [iDegToTurn, FTurnRate, iExpectedTurnTime]));

  if iExpectedTurnTime > AMaxTurnTime then
    iExpectedTurnTime := AMaxTurnTime;

  if iDegToTurn < 0 then begin
    SetArrowLeft(true);
    sleep(iExpectedTurnTime);
    SetArrowLeft(false);
  end
  else begin
    SetArrowRight(true);
    sleep(iExpectedTurnTime);
    SetArrowRight(false);
  end;
end;

procedure TDAOCControl.TurnRateNeeded;
begin
    { calibrate the turn rate if we don't know it }
  if FTurnRate = 0 then
    TurnRateRecalibrate;
end;

procedure TDAOCControl.Jump;
begin
  SendVKDown(Ord('A'), true);
  sleep(100);
  SendVKDown(Ord('A'), false);
end;

procedure TDAOCControl.TurnRateRecalibrate;
var
  iTries:   integer;
begin
  FTurnRate := 0;
  
  AllKeysUp;
  sleep(1000);  // make sure we're not moving still
    { This basically set the requirement that we get TURNRATE_CAL_COUNT loc
      packets to calibrate the turnrate }
  FTurnRateCalLeft := TURNRATE_CAL_COUNT;
  iTries := 20;
  SetArrowRight(true);
  while (FTurnRateCalLeft > 0) and (iTries > 0) do begin
    sleep(1000);
    dec(iTries);
  end;
  SetArrowRight(false);
  sleep(1000);  // make sure we're not moving still
end;

procedure TDAOCControl.GotoXY(X, Y: DWORD);
begin
  ClearGotoDest;
  FDestGotoNode := TMapNode.Create;
  FDestGotoNode.Region := FRegionID;
  FDestGotoNode.X := X;
  FDestGotoNode.Y := Y;
  FDestGotoNodeFreeable := true;

  DoTurntoDest(1000);
end;

procedure TDAOCControl.NodeAddAtPlayerPos(const AName: string);
var
  pNode:  TMapNode;
begin
  pNode := TMapNode.Create;
  pNode.Name := AName;
  pNode.Region := FRegionID;
  pNode.X := FLocalPlayer.X;
  pNode.Y := FLocalPlayer.Y;
  pNode.Z := FLocalPlayer.Z;
  pNode.Head := FLocalPlayer.Head;
  pNode.Radius := FMapNodes.DefaultRadius;
  try
    FMapNodes.Add(pNode);
  except
    pNode.Free;
    raise;
  end;
end;

function TDAOCControl.NodeClosestToPlayerPos: TMapNode;
begin
  Result := FMapNodes.FindNearestNode3D(FLocalPlayer.X, FLocalPlayer.Y, FLocalPlayer.Z, false);
end;

procedure TDAOCControl.LeftClick(X, Y: Integer);
var
  hFocusWnd:  HWND;
begin
  hFocusWnd := GetForegroundWindow;
  if hFocusWnd <> FMainHWND then begin
    AdjustWindowCoords(X, Y);
    SetCursorPos(X, Y);
    sleep(100);
    Windows.SendMessage(hFocusWnd, WM_LBUTTONDOWN, MK_LBUTTON, MakeLParam(X, Y));
    sleep(150);
    Windows.SendMessage(hFocusWnd, WM_LBUTTONUP, 0, MakeLParam(X, Y));
  end;
end;


procedure TDAOCControl.RightClick(X, Y: Integer);
var
  hFocusWnd:  HWND;
begin
  hFocusWnd := GetForegroundWindow;
  if hFocusWnd <> FMainHWND then begin
    AdjustWindowCoords(X, Y);
    SetCursorPos(X, Y);
    sleep(100);
    Windows.SendMessage(hFocusWnd, WM_RBUTTONDOWN, MK_RBUTTON, MakeLParam(X, Y));
    sleep(150);
    Windows.SendMessage(hFocusWnd, WM_RBUTTONUP, 0, MakeLParam(X, Y));
  end;
end;

procedure TDAOCControl.GotoNode(ANode: TMapNode);
begin
  SetGotoDest(ANode);
  Log('Going to node: ' + FDestGotoNode.Name);
  DoTurntoDest(1000);
end;

procedure TDAOCControl.DoOnArriveAtGotoDest;
begin
  StopGotoDest;

    { see if this node was just one in a path }
  if ContinueCurrentPath then
    exit;

  if FAutomationMode = amCommission then begin
    // if haveitem then dropoff item
    // if NoComisssion get commission
    // if gathering materials, open merchant window
    // if readytobuild, build item
  end;  { if AutomationMode commission }

  if Assigned(FOnArriveAtGotoDest) then
    FOnArriveAtGotoDest(Self);

  if Assigned(FAxEvents) then
    FAxEvents.OnArriveAtGotoDest;
end;

procedure TDAOCControl.ClearGotoDest;
begin
  if FDestGotoNodeFreeable then
    FreeAndNil(FDestGotoNode)
  else
    FDestGotoNode := nil;
  FDestGotoNodeFreeable := false;
end;

procedure TDAOCControl.DoGotoDest;
const
  PLAYER_DECELERATION_RATE = 300;
var
  iDist:  integer;
  iCoastTime: integer;
  iETE:   integer;
begin
  if WhithinToleranceOfDest then begin
    DoOnArriveAtGotoDest;
    exit;
  end;

  if StuckGoingToDest then begin
    StopAllActions;
    Log('Got stuck going to dest!');
    exit;
  end;

  iDist := FDestGotoNode.Distance3D(FLocalPlayer.XProjected, FLocalPlayer.YProjected, FLocalPlayer.Z);
    { coast time = time it will take player to stop (in 1/100ths of a sec) }
  iCoastTime := (AbsI(FLocalPlayer.Speed) * 100) div PLAYER_DECELERATION_RATE;
    { ETE = estimated time en route to destination (in 1/100ths of a sec) }
  if FLocalPlayer.Speed = 0 then
    iETE := MAXINT
  else
    iETE := (iDist * 100) div FLocalPlayer.Speed;

    { move / continue move only if were:
      -- Within ForceStationaryTurnDegrees degrees of facing the right direction
      -- Either
         Can turn to the next path node without stopping
           or
         Ouside the coast distance to our destination
    }
  ArrowUp :=
      (AbsI(PlayerToHeadingDelta(BearingToDest)) < FForceStationaryTurnDegrees)
    and
      (NextNodeInPathIsCoastTurnable
        or
      (iETE > iCoastTime));

    { only turn as much as we can be guaranteed not have the position updates
      start stacking up.  Since we get a pos update every second or so,
      it should be safe to turn up to 333ms at a time }
  DoTurntoDest(333);
end;

procedure TDAOCControl.StopGotoDest;
begin
  ArrowUp := NextNodeInPathIsCoastTurnable;
  ClearGotoDest;
end;

function TDAOCControl.PlayerToHeadingDelta(AHead: integer) : integer;
(*** Returns the degrees to turn [-180..180] to get the player's heading to AHead ***)
begin
  AHead := AHead mod 360;
  if AHead < 0 then
    inc(AHead, 360);
  Result := AHead - FLocalPlayer.Head;
  if Result > 180 then
    dec(Result, 360);
  if Result < -180 then
    inc(Result, 360);
end;

procedure TDAOCControl.DoTurntoDest(AMaxTurnTime: integer);
begin
  SetPlayerHeading(BearingToDest, AMaxTurnTime);
end;

function TDAOCControl.BearingToDest: integer;
begin
  Result := FDestGotoNode.BearingFrom(FLocalPlayer.XProjected, FLocalPlayer.YProjected);
end;

function TDAOCControl.PathToNode(ANode: TMapNode): boolean;
var
  pStartNode:   TMapNode;
  pPath:        TMapNodeList;
begin
  pStartNode := ConnectedNodeClosestToPlayerPos;
  if not Assigned(pStartNode) then begin
    Result := false;
    exit;
  end;

  pPath := pStartNode.FindPathTo(ANode);
  if not Assigned(pPath) then begin
    Result := false;
    exit;
  end;

  ClearCurrentPath;

  FCurrentPath := pPath;
  FCurrentPathIdx := 0;
  DoOnPathChanged;

  Log('Pathing from ' + pStartNode.Name + ' -> ' + ANode.Name);
  ContinueCurrentPath;
  Result := true;
end;

procedure TDAOCControl.ClearCurrentPath;
begin
  if Assigned(FCurrentPath) then begin
    FreeAndNil(FCurrentPath);
    FCurrentPathIdx := -1;

    DoOnPathChanged;
  end;
end;

procedure TDAOCControl.SetDAOCPath(const Value: string);
begin
{$IFDEF VER130}
  FDAOCPath := IncludeTrailingBackslash(Value);
{$ELSE}
  FDAOCPath := IncludeTrailingPathDelimiter(Value);
{$ENDIF}
  FWindowManager.DAOCPath := FDAOCPath;
end;

procedure TDAOCControl.LeftClickEvent(Sender: TObject; X, Y: integer);
begin
  LeftClick(X, Y);
end;

procedure TDAOCControl.RightClickEvent(Sender: TObject; X, Y: integer);
begin
  RightClick(X, Y);
end;

procedure TDAOCControl.SendKeysEvent(Sender: TObject; const AKeys: string);
begin
  DoSendKeys(AKeys);
end;

procedure TDAOCControl.DoOnCharacterLogin;
begin
  FWindowManager.CharacterName := FLocalPlayer.Name;
  inherited DoOnCharacterLogin;

  if Assigned(FAxEvents) then
    FAxEvents.OnCharacterLogin;
end;

procedure TDAOCControl.DoOnConnect;
begin
  FWindowManager.ServerIP := ServerIP;
  FSamePlayerPosCount := 0;

  inherited DoOnConnect;

  if Assigned(FAxEvents) then
    FAxEvents.OnConnect;
end;

procedure TDAOCControl.FaceNode(ANode: TMapNode; AMaxTurnTime: integer);
begin
  SetPlayerHeading(ANode.BearingFrom(FLocalPlayer.X, FLocalPlayer.Y), AMaxTurnTime);
end;

function TDAOCControl.StepCurrentPath: boolean;
begin
  inc(FCurrentPathIdx);
  if FCurrentPathIdx < FCurrentPath.Count then begin
    SetGotoDest(FCurrentPath[FCurrentPathIdx]);
    Result := true;
  end
  else begin
    ClearCurrentPath;
    Result := false;
  end;
end;

function TDAOCControl.WhithinToleranceOfDest: boolean;
begin
  Result := not Assigned(FDestGotoNode) or
    (FDestGotoNode.Distance3D(FLocalPlayer.XProjected, FLocalPlayer.YProjected, FLocalPlayer.Z) < FDestGotoNode.Radius);
end;

procedure TDAOCControl.CloseDialog;
begin
  TDialogWindow.CloseDialog(FWindowManager);
end;

procedure TDAOCControl.DoOnTradeskillSuccess(AQuality: integer);
begin
  if FTradeSkillProgression <> '' then begin
    if (FTradeSkillOddsloadKey <> '') and (FTradeskillTargetQuality > 0)
      and LastItemOfTradeskillProgressionIsNext then begin
      if LoadOddsPctContinue then
        exit;
      if LoadOddsStreakContinue then
        exit;
    end;  { if loadodds and targetqual > 0 and on last combine step }

    Log(Format('%8.8x Tradeskill Item [%s] complete %d%%q',
        [GetTickCount, FTradeSkillProgression[FTradeSkillProgressionIdx],
        AQuality]));

    if FTradeSkillProgressionIdx = Length(FTradeSkillProgression) then begin
      FTradeSkillProgressionIdx := 1;

      if (FTradeskillTargetQuality > 0) and (AQuality >= FTradeskillTargetQuality) then begin
        Log('Target tradeskill quality ' + IntToStr(FTradeskillTargetQuality) + '% reached.  Not continuing.');
        if  FTradeskillTargetSound <> '' then
          if AnsiSameText(FTradeskillTargetSound, 'beep') then begin
            Windows.Beep(1000, 250);
            Windows.Beep(500, 250);
            Windows.Beep(1000, 250);
            Windows.Beep(500, 250);
          end
          else
            PlaySound(PChar(FTradeskillTargetSound), 0, SND_ASYNC);
        exit;
      end;
    end  { if on last item in tradeskill progression }
    else
      inc(FTradeSkillProgressionIdx);

    if FTradeSkillStopIfFull and FLocalPlayer.Inventory.IsFull then begin
      Log('Inventory is full, not continuing tradeskill');
      exit;
    end;

    TradeskillContinueProgression;
  end;

    { the item goes in our inventory after the success message, so we need to wait }
//  if FAutomationMode = amCommission then
//    if TradeCommissionItem <> '' then
//      ScheduleCallback(750, TradeskillSuccessCallback, 0);
    { we call the inherited first in case after this success, someone
      wants to change the tradeskill progression }

  inherited;

  if Assigned(FAxEvents) then
    FAxEvents.OnTradeskillSuccess(AQuality);
end;

(***
procedure TDAOCControl.TradeskillSuccessCallback(Sender: TObject;
  AParm: LPARAM);
begin
  CheckHaveTradeCommissionItem;
end;
***)

procedure TDAOCControl.DoOnTradeskillTaskCompleted;
begin
  inherited;

  if FAutomationMode = amCommission then begin
    FCommissionAutomationState := casNoCommission;
    PathToNodeName(GetTradeMasterName);
  end;  { if automationmode commission }

  if Assigned(FAxEvents) then
    FAxEvents.OnTradeskillTaskCompleted;
end;

procedure TDAOCControl.SetAutomationMode(const Value: TAutomationMode);
begin
  FAutomationMode := Value;
end;

function TDAOCControl.GetTradeMasterName: string;
var
  I:  integer;
  iMaxSkill:  integer;
  iMaxSkillIdx:  integer;
begin
  iMaxSkillIdx := -1;
  iMaxSkill := 0;
  for I := 0 to LocalPlayer.Skills.Count - 1 do
    if (iMaxSkillIdx = -1) or (LocalPlayer.Skills[I].Value > iMaxSkill) then begin
      iMaxSkill := LocalPlayer.Skills[I].Value;
      iMaxSkillIdx := I;
    end;

  if iMaxSkillIdx = -1 then
    Result := ''
  else if AnsiSameText(LocalPlayer.Skills[iMaxSkillIdx].Name, 'Weaponcraft') then
    Result := 'WeapMaster'
  else if AnsiSameText(LocalPlayer.Skills[iMaxSkillIdx].Name, 'Armorcraft') then
    Result := 'ArmorMaster'
  else if AnsiSameText(LocalPlayer.Skills[iMaxSkillIdx].Name, 'Tailoring') then
    Result := 'TailorMaster'
  else if AnsiSameText(LocalPlayer.Skills[iMaxSkillIdx].Name, 'Fletching') then
    Result := 'FletchMaster'
  else
    Result := '';
end;

procedure TDAOCControl.DoOnTradeCommissionAssigned;
begin
  inherited;

  if FAutomationMode = amCommission then begin
    FCommissionAutomationState := casGotCommission;

    if CheckHaveTradeCommissionItem then
      exit;

    if CheckNeedMaterials then
      exit;

    FCommissionAutomationState := casReadyToBuild;
    if not PathToNodeName('Forge1') then
      Log('ERROR: Cannot path to forge from here!');
  end; { FAutomationMode = amCommission }

  if Assigned(FAxEvents) then
    FAxEvents.OnTradeskillCommissionAssigned;
end;

procedure TDAOCControl.DoOnPopupMessage(const AMessage: string);
begin
  inherited;
  
  if FAutomationMode <> amNone then
    CloseDialog;

  if Assigned(FAxEvents) then
    FAxEvents.OnPopupMessage(AMessage);
end;

procedure TDAOCControl.Clear;
begin
  SaveMasterVendorListForRegion(FRegionID);
  inherited;
  FCommissionAutomationState := casUnknown;
  FTradeSkillProgressionIdx := 1;  
end;

function TDAOCControl.CheckHaveTradeCommissionItem : boolean;
begin
  if (TradeCommissionItem <> '') and LocalPlayer.Inventory.HasItem(TradeCommissionItem) then begin
    Log('Commission item ' + TradeCommissionItem + ' complete');
    FCommissionAutomationState := casHaveItem;

    if not PathToNodeName(TradeCommissionNPC + '1') then
      Log('ERROR: Can''t find node for commission delivery: ' + TradeCommissionNPC);

    Result := true;
  end  { if commission item completed }
  else
    Result := false;
end;

function TDAOCControl.CheckNeedMaterials: boolean;
begin
  Result := false;  
end;

function TDAOCControl.PathToNodeName(const ANodeName: string): boolean;
var
  pNode:  TMapNode;
begin
  pNode := FMapNodes.NodeByName(ANodeName);
  if not Assigned(pNode) then begin
    Result := false;
    exit;
  end;

  Result := PathToNode(pNode);
end;

procedure TDAOCControl.AdjustWindowCoords(var X, Y: integer);
var
  hDAOCWnd: HWND;
  P:    TPoint;
begin
  hDAOCWnd := FindDAOCWindow;
  if hDAOCWnd <> 0 then begin
    P.X := X;
    P.Y := Y;
    ClientToScreen(hDAOCWnd, P);
    X := P.X;
    Y := P.Y;
  end;
end;

function TDAOCControl.ConnectedNodeClosestToPlayerPos: TMapNode;
begin
  Result := FMapNodes.FindNearestNode3D(FLocalPlayer.X, FLocalPlayer.Y, FLocalPlayer.Z, true);
end;

function TDAOCControl.NextNodeInPath: TMapNode;
begin
  if (FCurrentPathIdx = -1) or not Assigned(FCurrentPath) or
    ((FCurrentPathIdx+1) >= FCurrentPath.Count) then
    Result := nil
  else
    Result := FCurrentPath[FCurrentPathIdx + 1];
end;

function TDAOCControl.NextNodeInPathIsCoastTurnable: boolean;
var
  pNextNodeInPath:  TMapNode;
begin
  pNextNodeInPath := NextNodeInPath;
  Result := Assigned(FDestGotoNode) and Assigned(pNextNodeInPath) and
    (AbsI(PlayerToHeadingDelta(FDestGotoNode.BearingTo(pNextNodeInPath))) < FForceStationaryTurnDegrees);

//  if Assigned(pNextNodeInPath) then
//    Log(Format('Next node coastturnable=%d bearing %d headingdelta %d', [
//      ord(Result), FDestGotoNode.BearingTo(pNextNodeInPath),
//      PlayerToHeadingDelta(FDestGotoNode.BearingTo(pNextNodeInPath))])
//    );
end;

procedure TDAOCControl.ContinueTurnRateCal;
var
  iDegreesTurned: integer;
  dwTicks:  DWORD;
begin
    { discard the first cal value }
  if FTurnRateCalLeft < TURNRATE_CAL_COUNT then begin
    { we calibrate by turning right, so the new head should always be more
      than the old head }
    iDegreesTurned := FLocalPlayer.Head - FLastPlayerPos.Head;
    if iDegreesTurned < 0 then
      inc(iDegreesTurned, 360);
    dwTicks := FLocalPlayer.LastUpdate - FLastPlayerPos.LastUpdate;
    if iDegreesTurned <> 0 then
      FTurnRate := FTurnRate + integer(dwTicks * 100) div iDegreesTurned;
    Log('Turn rate calibrate '+ IntToStr(FTurnRateCalLeft) + ': ' +
      IntToStr(iDegreesTurned) + ' in ' + IntToStr(dwTicks) + ' ms');
  end;

  dec(FTurnRateCalLeft);
    { get the average if the values if we are done }
  if FTurnRateCalLeft = 0 then
    FTurnRate := FTurnRate div (TURNRATE_CAL_COUNT - 1);
end;

procedure TDAOCControl.SetGotoDest(ANode: TMapNode);
begin
  ClearGotoDest;
  FDestGotoNode := ANode;
  FDestGotoNodeFreeable := false;
end;

function TDAOCControl.ContinueCurrentPath: boolean;
(***
  Returns true if we're going to the next node.  Resturns false if there
  is no path or we're out of path nodes.
  If we're supposed to continue the path then go.
***)
begin
  Result := false;
  if Assigned(FCurrentPath) then begin
      { loop until we find a node that we are not close enough to }
    while WhithinToleranceOfDest do
      if not StepCurrentPath then
        exit;

    GotoNode(FDestGotoNode);
    Result := true;
  end;  { if FCurrentPath }
end;

function TDAOCControl.StuckGoingToDest: boolean;
begin
  Result := Assigned(FDestGotoNode) and ArrowUp and
    (FSamePlayerPosCount > 3);
end;

procedure TDAOCControl.DoOnPathChanged;
begin
  if Assigned(FOnPathChanged) then
    FOnPathChanged(Self);

  if Assigned(FAxEvents) then
    FAxEvents.OnPathChanged;
end;

procedure TDAOCControl.DoOnTradeskillCapped;
begin
  if FTradeSkillProgression <> '' then begin
    Log(Format('Tradeskill capped! Progression "%s" Index %d', [
      FTradeSkillProgression, FTradeSkillProgressionIdx]));
      { stop the current item from building }
    DoSendKeys(FTradeSkillProgression[FTradeSkillProgressionIdx]);
  end;
  
  inherited;

  if Assigned(FAxEvents) then
    FAxEvents.OnTradeskillCapped;
end;

procedure TDAOCControl.DoOnTradeskillFailure;
begin
  if FTradeSkillProgression <> '' then
    DoSendKeys(FTradeSkillProgression[FTradeSkillProgressionIdx]);

  inherited;

  if Assigned(FAxEvents) then
    FAxEvents.OnTradeskillFailure;
end;

function TDAOCControl.FindDAOCWindow: HWND;
begin
  Result := FindWindow('DAoCMWC', nil); // nil, 'Dark Age of Camelot,  (c) 2002 Mythic Entertainment, Inc.');
end;

procedure TDAOCControl.SendOneKeyNoFocus(c: char);
var
  MKey: integer;
  VKey: BYTE;
  ScanCode: byte;
  Flags:  DWORD;
begin
  if FDAOCHWND <> 0 then begin
    MKey := VkKeyScan(c);
    VKey := Lo(MKey);
    ScanCode := Lo(MapVirtualKey(VKey, 0));
    Flags := 0;

    Flags := Flags or (ScanCode shl 16);
    SendMessage(FDAOCHWND, WM_KEYDOWN, VKey, Flags);
    SendMessage(FDAOCHWND, WM_CHAR, ord(c), Flags);
    Flags := Flags or $80000000;
    SendMessage(FDAOCHWND, WM_KEYUP, Vkey, LParam(Flags));
  end
  else
    Log('SendOneKeyNoFocus: Window not found');
end;

procedure TDAOCControl.SetTradeSkillProgression(const Value: string);
begin
  if FTradeSkillProgression = Value then
    exit;

  FTradeSkillProgression := Value;
  FTradeSkillProgressionIdx := 1;
Log('Resetting TradeskillProgression ' + IntToStr(FTradeSkillProgressionIdx));
end;

procedure TDAOCControl.DoOnChatSendIncoming(const AWho, AMessage: string);
begin
  if FAFKMessage <> '' then
    DoSendKeys('/send ' + AWho + ' ' + FAFKMessage + '[cr]');

  inherited;

  if Assigned(FAxEvents) then
    FAxEvents.OnChatSendIncoming(AWho, AMessage);
end;

procedure TDAOCControl.DoOnDisconnect;
begin
  SaveMasterVendorListForRegion(FRegionID);
  inherited;

  if Assigned(FAxEvents) then
    FAxEvents.OnDisconnect;
end;

procedure TDAOCControl.DoSetRegionID(ARegion: integer);
begin
  SaveMasterVendorListForRegion(FRegionID);
  inherited;
  LoadMasterVendorListForRegion(FRegionID);
end;

procedure TDAOCControl.LoadMasterVendorListForRegion(ARegionID: integer);
var
  sFileName:  string;
begin
  sFileName := Format('region%3.3d.vend', [ARegionID]);
  if FileExists(sFileName) then
    FMasterVendorList.LoadFromFile(sFileName)
  else
    FMasterVendorList.Clear;
end;

procedure TDAOCControl.SaveMasterVendorListForRegion(ARegionID: integer);
var
  sFileName:  string;
begin
  if FMasterVendorList.Count > 0 then begin
    sFileName := Format('region%3.3d.vend', [ARegionID]);
    FMasterVendorList.SaveToFile(sFileName);
  end;
end;

procedure TDAOCControl.UpdateDAOCWindowHnd;
begin
  FDAOCHWND := FindDAOCWindow;
end;

procedure TDAOCControl.EventSinkChanged(const EventSink: IInterface);
begin
  FAxEvents := EventSink as IDAOCControlEvents;
end;

procedure TDAOCControl.Initialize;
begin
  inherited Initialize;
  
  FConnectionPoints := TConnectionPoints.Create(Self);
  if AutoFactory.EventTypeInfo <> nil then
    FConnectionPoint := FConnectionPoints.CreateConnectionPoint(
      AutoFactory.EventIID, ckSingle, EventConnect)
  else FConnectionPoint := nil;
end;

function TDAOCControl.GetClassTypeInfo: ITypeInfo;
begin
  Result := AutoFactory.ClassInfo;
end;

procedure TDAOCControl.DAOCCSleep(dwTime: integer);
begin
  Windows.Sleep(dwTime);
end;

procedure TDAOCControl.SelectGroupMember(AIndex: integer);
begin
  SendVKDown(VK_SHIFT, true);
  SendKeys(PChar('[F' + IntToStr(AIndex) + ']'), true);
  Sleep(500);
  SendVKDown(VK_SHIFT, false);
end;

procedure TDAOCControl.DoOnSelectedObjectChanged(AObject: TDAOCObject);
begin
  inherited;

  if Assigned(FAxEvents) then
    FAxEvents.OnSelectedObjectChanged;
end;

procedure TDAOCControl.DAOCCLog(const AMessage: WideString);
begin
  Log(AMessage);
end;

procedure TDAOCControl.Face;
begin
  DoSendKeys('/face[cr]');
end;

procedure TDAOCControl.Follow;
begin
  DoSendKeys('/follow[cr]');
end;

procedure TDAOCControl.Stick;
begin
  DoSendKeys('/stick[cr]');
end;

procedure TDAOCControl.ChatAlliance(const AMessage: string);
begin
  DoSendKeys('/as ' + AMessage + '[cr]');
end;

procedure TDAOCControl.ChatChat(const AMessage: string);
begin
  DoSendKeys('/c ' + AMessage + '[cr]');
end;

procedure TDAOCControl.ChatGroup(const AMessage: string);
begin
  DoSendKeys('/g ' + AMessage + '[cr]');
end;

procedure TDAOCControl.ChatGuild(const AMessage: string);
begin
  DoSendKeys('/gu ' + AMessage + '[cr]');
end;

procedure TDAOCControl.ChatSay(const AMessage: string);
begin
  DoSendKeys('/s ' + AMessage + '[cr]');
end;

procedure TDAOCControl.ChatSend(const AWho, AMessage: string);
begin
  DoSendKeys('/send ' + AWho + ' ' + AMessage + '[cr]');
end;

procedure TDAOCControl.ChatAllianceW(const bsMessage: WideString);
begin
  ChatAlliance(bsMessage);
end;

procedure TDAOCControl.ChatChatW(const bsMessage: WideString);
begin
  ChatChat(bsMessage);
end;

procedure TDAOCControl.ChatGroupW(const bsMessage: WideString);
begin
  ChatGroup(bsMessage);
end;

procedure TDAOCControl.ChatGuildW(const bsMessage: WideString);
begin
  ChatGuild(bsMessage);
end;

procedure TDAOCControl.ChatSayW(const bsMessage: WideString);
begin
  ChatSay(bsMessage);
end;

procedure TDAOCControl.ChatSendW(const bsWho: WideString; const bsMessage: WideString);
begin
  ChatSend(bsWho, bsMessage);
end;

function TDAOCControl.LastItemOfTradeskillProgressionIsNext: boolean;
begin
  Result := (Length(FTradeSkillProgression) = 1) or
    (FTradeSkillProgressionIdx = Length(FTradeSkillProgression) - 1);
end;

function TDAOCControl.LoadOddsStreakContinue : boolean;
begin
  if (FTradeSkillOddsloadPct = 0) and (FTradeSkillOddsloadCount > 0) and
    (FChatParser.TradeSkillStreaks[FTradeskillTargetQuality] <= FTradeSkillOddsloadCount) then begin
    Log(Format('Loading the odds: %d of %d', [
      FChatParser.TradeSkillStreaks[FTradeskillTargetQuality],
      FTradeSkillOddsloadCount]));

    DoSendKeys(FTradeSkillOddsloadKey[1]);
    Result := true
  end  { if streak-based odds loading }
  else
    Result := false;
end;

function TDAOCControl.LoadOddsPctContinue : boolean;
var
  I:    integer;
  iPct: integer;
begin
  if FChatParser.TradeSkillQualityDistribution[101] > 0 then begin
    iPct := 0;
    for I := FTradeskillTargetQuality to 100 do
      inc(iPct, FChatParser.TradeSkillQualityDistribution[I]);

    iPct := iPct * 1000 div FChatParser.TradeSkillQualityDistribution[101];
  end
  else
    iPct := 100;

  if FChatParser.TradeSkillQualityDistribution[101] <= FTradeSkillOddsloadCount then begin
    Log(Format('Not enough samples for OddsLoadPct: %d of %d (pct %0.1f%%)',
      [FChatParser.TradeSkillQualityDistribution[101], FTradeSkillOddsloadCount,
      iPct / 10]));
    DoSendKeys(FTradeSkillOddsloadKey[1]);
    Result := true;
    exit;
  end;

  if (FTradeSkillOddsloadPct > 0) and (iPct >= FTradeSkillOddsloadPct) then begin
    Log(Format('Loading the odds: %0.1f%% of %0.1f%%', [
      iPct / 10, FTradeSkillOddsloadPct / 10]));

    DoSendKeys(FTradeSkillOddsloadKey[1]);
    Result := true;
  end  { if streak-based odds loading }
  else
    Result := false;
end;

procedure TDAOCControl.FocusDAOCWindow;
begin
  UpdateDAOCWindowHnd;
  W2K_SetForegroundWindow(FDAOCHWND);
end;

function TDAOCControl.GetTradeRecipes: TUniversalRecipeCollection;
var
  mpkIFD:   TMPKFile;
  strmIRF:  TStream;
begin
  if not Assigned(FTradeRecipes) then begin
    FTradeRecipes := TUniversalRecipeCollection.Create;
(**
    if FileExists('tdl.bin') then
      FTradeRecipes.LoadFromFileBIN('tdl.bin')
    else if FileExits('tdl.irf') then begin
      FTradeRecipes.LoadFromFileIRF('tdl.irf');
      FTradeRecipes.SaveToFileBIN('tdl.bin');
    end
    else begin
**)
      mpkIFD := TMPKFile.Create(FDAOCPath + 'data\ifd.mpk');
      strmIRF := mpkIFD.ExtractStream('tdl.irf');
      FTradeRecipes.LoadFromStreamIRF(strmIRF);
//    end;
  end;

  Result := FTradeRecipes;
end;

procedure TDAOCControl.DoVKDown(vk: byte);
begin
  SendVKDown(vk, true);
end;

procedure TDAOCControl.DoVKUp(vk: byte);
begin
  SendVKDown(vk, false);
end;

procedure TDAOCControl.SendVKDownEvent(Sender: TObject; vk: byte);
begin
  DoVKDown(vk);
end;

procedure TDAOCControl.SendVKUpEvent(Sender: TObject; vk: byte);
begin
  DoVKUp(vk);
end;

procedure TDAOCControl.TradeskillContinueProgression;
begin
  if FTradeSkillProgression <> '' then
    DoSendKeys(FTradeSkillProgression[FTradeSkillProgressionIdx]);
end;

procedure TDAOCControl.DoOnCombatStyleFailure;
begin
  inherited;
  if Assigned(FAxEvents) then
    FAxEvents.OnCombatStyleFailure;;
end;

procedure TDAOCControl.DoOnCombatStyleSuccess(AStyle: string);
begin
  inherited;
  if Assigned(FAxEvents) then
    FAxEvents.OnCombatStyleSuccess(AStyle);
end;

initialization
  TAutoObjectFactory.Create(ComServer, TDAOCControl, CLASS_CDAOCControl,
    ciInternal, tmFree);

end.
