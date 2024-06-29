#pragma once

struct ReticlePair
{
	unsigned short TableValue;
	unsigned short TableOffset;
};

const pair<wchar_t* , wchar_t*> BonePairs[ ]
{
	{L"neck_01", L"upperarm_r"},
	{L"neck_01", L"upperarm_l"},
	{L"upperarm_l", L"lowerarm_l"},
	{L"lowerarm_l", L"hand_l"},
	{L"upperarm_r", L"lowerarm_r"},
	{L"lowerarm_r", L"hand_r"},
	{L"neck_01", L"pelvis"},
	{L"pelvis", L"thigh_r"},
	{L"pelvis", L"thigh_l"},
	{L"thigh_r", L"calf_r"},
	{L"calf_r", L"ik_foot_r"},
	{L"thigh_l", L"calf_l"},
	{L"calf_l", L"ik_foot_l"}
};

vector<pair<wchar_t* , wchar_t*>> SkeletonBonePairs( BonePairs , sizeof( BonePairs ) / sizeof( pair<wchar_t* , wchar_t*> ) );

bool PositiveCurve = false;

void ( *DrawTransitionOriginal )( SDK::UGameViewportClient*, SDK::UCanvas* );
void DrawTransition( SDK::UGameViewportClient* ViewportClient , SDK::UCanvas* Canvas )
{
	if ( !ViewportClient || !Canvas ) return DrawTransitionOriginal( ViewportClient , Canvas );

	Implementation::Canvas = Canvas;
	Implementation::ScreenSize = SDK::FVector2D( Canvas->ClipX( ) , Canvas->ClipY( ) );
	Implementation::ScreenCenter = SDK::FVector2D( Implementation::ScreenSize.X / 2.0 , Implementation::ScreenSize.Y / 2.0 );
	double LegacyFontSize = ( double ) ( Implementation::Font->LegacyFontSize( ) ) + 2.0; // Text Offset

	SDK::FVector LocalHeadLocation = SDK::FVector( );

	bool Update__GetControlRotation__bAimbotActive = false;
	SDK::FRotator Update__GetControlRotation__AimbotRotationTarget = SDK::FRotator( );
	SDK::FVector Update__GetControlRotation__AimbotLocationTarget = SDK::FVector( );

	bool Update__GetWeaponTargetingTransform__bSilentAimActive = false;
	SDK::FVector Update__GetWeaponTargetingTransform__SilentLocationTarget = SDK::FVector( );

	SDK::UWorld* World = ViewportClient->World( );
	if ( !World ) return DrawTransitionOriginal( ViewportClient , Canvas );
	SDK::FVector CameraLocation = World->GetCameraLocation( );
	SDK::FRotator CameraRotation = World->GetCameraRotation( );
	double WorldDeltaSeconds = SDK::GameplayStatics->GetWorldDeltaSeconds( World );

	SDK::ULevel* PersistentLevel = World->PersistentLevel( );
	if ( !PersistentLevel ) return DrawTransitionOriginal( ViewportClient , Canvas );

	SDK::UGameInstance* OwningGameInstance = World->OwningGameInstance( );
	if ( !OwningGameInstance ) return DrawTransitionOriginal( ViewportClient , Canvas );

	SDK::ULocalPlayer* LocalPlayer = OwningGameInstance->LocalPlayers( )[ 0 ];
	if ( !LocalPlayer ) return DrawTransitionOriginal( ViewportClient , Canvas );

	SDK::APlayerController* PlayerController = LocalPlayer->PlayerController( );
	if ( !PlayerController ) return DrawTransitionOriginal( ViewportClient , Canvas );
	Implementation::PlayerController = PlayerController;
	Exploits::Hook( PlayerController );

	if ( PlayerController->WasInputKeyJustPressed( Implementation::F8 ) )
	{
		ViewportClientHook.Unhook( ViewportClient , DrawTransitionOriginal );
	}

	SDK::USceneViewState* ViewState = LocalPlayer->GetViewState( );
	if ( !ViewState ) return DrawTransitionOriginal( ViewportClient , Canvas );

	float FieldOfView = SDK::MathLibrary->atan( 1.0 / *( double* ) ( ViewState + 0x700 ) ) * 2.f;
	FieldOfView *= ( 180.f / M_PI );

	double AimbotFOV = ( Settings::FieldOfView * Implementation::ScreenSize.X / FieldOfView ) / 2.0;

	if ( Settings::FOVCircle )
	{
		Wrapper::Circle( Implementation::ScreenCenter , SDK::FLinearColor( 1.f , 1.f , 1.f , 1.f ) , AimbotFOV , 138.0 , false );
	}

	SDK::UFortClientSettingsRecord* ClientSettingsRecord = LocalPlayer->ClientSettingsRecord( );
	if ( !ClientSettingsRecord ) return DrawTransitionOriginal( ViewportClient , Canvas );

	SDK::APlayerPawn_Athena_C* AcknowledgedPawn = PlayerController->AcknowledgedPawn( );

	double ClosestDistanceToCenter = DBL_MAX;
	SDK::APlayerPawn_Athena_C* TargetClosestToCenter = nullptr;

	SDK::TArray<SDK::UObject*> PlayerArray = SDK::GameplayStatics->GetAllActorsOfClass( World , Implementation::Player );
	for ( int i = 0; i < PlayerArray.NumElements; i++ )
	{
		if ( !PlayerArray.IsValid( ) ) continue;
		if ( !PlayerArray.IsValidIndex( i ) )
			continue;

		SDK::APlayerPawn_Athena_C* Player = reinterpret_cast< SDK::APlayerPawn_Athena_C* >( PlayerArray[ i ] );
		if ( !Player )
			continue;

		bool ObjectIsA = SDK::GameplayStatics->ObjectIsA( Player , Implementation::Player );
		if ( !ObjectIsA ) continue;

		if ( Player == AcknowledgedPawn ) continue;

		bool Wound = Player->IsDBNO( );
		bool Dead = Player->IsDead( );
		if ( Dead ) continue;

		bool Teammate = SDK::FortLibrary->OnSameTeam( Player , AcknowledgedPawn );
		if ( Teammate ) continue;

		SDK::USkeletalMeshComponent* Mesh = Player->Mesh( );
		if ( !Mesh )
			continue;

		SDK::FVector2D HeadPosition = SDK::FVector2D( );
		SDK::FVector HeadLocation = Custom::GetSocketLocation( Mesh , SDK::FString( L"head" ) );
		if ( !Custom::K2_Project( HeadLocation , &HeadPosition ) ) continue;

		SDK::FVector2D BasePosition = SDK::FVector2D( );
		SDK::FVector BaseLocation = Custom::GetSocketLocation( Mesh , SDK::FString( L"root" ) );
		if ( !Custom::K2_Project( BaseLocation , &BasePosition ) ) continue;
		double Distance = SDK::MathLibrary->Vector_Distance( CameraLocation , BaseLocation ) * 0.01;

		if ( Custom::InCircle( AimbotFOV , HeadPosition ) )
		{
			if ( Distance <= Settings::AimbotDistance )
			{
				double DistanceToCenter = SDK::MathLibrary->Vector_Distance2D( Implementation::ScreenCenter , HeadPosition );
				if ( DistanceToCenter < ClosestDistanceToCenter )
				{
					TargetClosestToCenter = Player;
					ClosestDistanceToCenter = DistanceToCenter;
				}
			}
		}

		if ( Settings::Visuals )
		{
			if ( !Custom::InScreen( HeadPosition ) ) 
				continue;

			SDK::APlayerState* PlayerState = Player->PlayerState( );
			if ( !PlayerState )
				continue;

			bool IsABot = PlayerState->IsABot( );

			SDK::FVector Origin = SDK::FVector( );
			SDK::FVector Extent = SDK::FVector( );
			float SphereRadius = 0.f;

			SDK::SystemLibrary->GetComponentBounds( Mesh , &Origin , &Extent , &SphereRadius );

			double MostLeft = DBL_MAX;
			double MostRight = DBL_MIN;
			double MostTop = DBL_MAX;
			double MostBottom = DBL_MIN;

			for ( int i = 0; i < 8; i++ )
			{
				SDK::FVector Corner = SDK::FVector( );

				if ( i & 1 )
				{
					Corner.X = 1.0 * Extent.X;
				}
				else
				{
					Corner.X = -1.0 * Extent.X;
				}

				if ( i & 2 )
				{
					Corner.Y = 1.0 * Extent.Y;
				}
				else
				{
					Corner.Y = -1.0 * Extent.Y;
				}

				if ( i & 2 )
				{
					Corner.Z = 1.0 * Extent.Z;
				}
				else
				{
					Corner.Z = -1.0 * Extent.Z;
				}

				SDK::FVector2D Origin2D = SDK::FVector2D( );
				if ( !Custom::K2_Project( Origin + Corner , &Origin2D ) ) continue;

				if ( Origin2D.X < MostLeft )
				{
					MostLeft = Origin2D.X;
				}

				if ( Origin2D.X > MostRight )
				{
					MostRight = Origin2D.X;
				}

				if ( Origin2D.Y < MostTop )
				{
					MostTop = Origin2D.Y;
				}

				if ( Origin2D.Y > MostBottom )
				{
					MostBottom = Origin2D.Y;
				}
			}

			double ActorHeight = MostBottom - MostTop;
			double ActorWidth = MostRight - MostLeft;

			double CornerWidth = ActorWidth / 5.0;
			double CornerHeight = ActorHeight / 5.0;

			double TopTextOffset = 17.0;
			double BottomTextOffset = 7.0;

			SDK::FVector2D BottomMiddle = SDK::FVector2D( MostLeft + ( ActorWidth / 2.0 ) , MostBottom );

			SDK::FLinearColor BoxColor = SDK::FLinearColor( 0.f , 0.8352941f , 1.f , 1.f );
			SDK::FLinearColor SkeletonColor = SDK::FLinearColor( 0.f , 0.8352941f , 1.f , 1.f );

			if ( !SDK::FortLibrary->CheckLineOfSightToActorWithChannel( CameraLocation , Player , SDK::ECollisionChannel::ECC_Visibility , AcknowledgedPawn ) )
			{
				SkeletonColor = BoxColor = SDK::FLinearColor( 1.f , 0.f , 0.f , 1.f );
			}

			if ( Settings::Radar )
			{
				double RadarSize = ( 15.625 * Implementation::ScreenCenter.X * double( ClientSettingsRecord->HUDScale( ) ) / 100.f ) * 2.0;

				SDK::FVector2D RadarPosition = SDK::FVector2D( 5.0 , 35.0 );

				Radar::Setup( RadarPosition , SDK::FVector2D( RadarSize , RadarSize ) );
				Radar::Add( BaseLocation , BoxColor , CameraLocation , CameraRotation , RadarPosition , SDK::FVector2D( RadarSize , RadarSize ) );
			}

			if ( Settings::Fill )
			{
				// No filled boxes for you
			}

			if ( Settings::Boxes == 1 )
			{
				// No 2D Boxes for you
			}
			else if ( Settings::Boxes == 2 )
			{
				// No conrer boxes for you
			}

			if ( Settings::Line )
			{
				Canvas->K2_DrawLine( HeadPosition , SDK::FVector2D( Implementation::ScreenCenter.X , 0.0 ) , 1.5f , SDK::FromRGB( 119 , 65 , 192 ) );
			}

			if ( Settings::Username )
			{
				SDK::FString Username = SDK::FortLibrary->GetHumanReadableName( Player );
				Username = IsABot ? SDK::FString( L"NPC/BOT" ) : Username;
				Username = Wound ? SDK::FString( L"Downed" ) : Username;
				Canvas->K2_DrawText( Implementation::Font , Username , SDK::FVector2D( BottomMiddle.X , MostTop - TopTextOffset ) , BoxColor , true , false , true );
			}

			if ( Settings::ActiveWeapon )
			{
				SDK::AFortWeapon* CurrentWeapon = Player->CurrentWeapon( );
				if ( CurrentWeapon )
				{
					bool IsHoldingWeapon = SDK::GameplayStatics->ObjectIsA( CurrentWeapon , Implementation::Weapon );
					if ( IsHoldingWeapon )
					{
						SDK::UFortWeaponItemDefinition* WeaponData = CurrentWeapon->WeaponData( );
						if ( WeaponData )
						{
							SDK::FString DisplayName = WeaponData->ItemName( ).Get( );
							if ( DisplayName )
							{
								SDK::FLinearColor RenderColor = BoxColor;
								int CurrentAmmo = CurrentWeapon->GetMagazineAmmoCount( );
								int MaxAmmo = CurrentWeapon->GetBulletsPerClip( );
								if ( CurrentAmmo && MaxAmmo )
								{
									RenderColor = Custom::GetColorByTier( WeaponData->Rarity( ) );
									DisplayName = SDK::StringLibrary->BuildString_Int( DisplayName , SDK::FString( L" (" ) , CurrentAmmo , SDK::FString( L"/" ) );
									DisplayName = SDK::StringLibrary->BuildString_Int( DisplayName , SDK::FString( ) , MaxAmmo , SDK::FString( L")" ) );
								}

								// No Weapon Icon ESP for you
								Canvas->K2_DrawText( Implementation::Font , DisplayName , SDK::FVector2D( BottomMiddle.X , MostBottom + BottomTextOffset ) , RenderColor , true , false , true );

								BottomTextOffset += LegacyFontSize;
							}
						}
					}
				}
			}

			if ( Settings::Distance )
			{
				SDK::FString ConvertedText = SDK::StringLibrary->BuildString_Double( SDK::FString( L"(" ) , SDK::FString( ) , int( Distance ) , SDK::FString( L")" ) );
				Canvas->K2_DrawText( Implementation::Font , ConvertedText , SDK::FVector2D( BottomMiddle.X , MostBottom + BottomTextOffset ) , BoxColor , true , false , true );
			}

			if ( Settings::Skeleton )
			{
				for ( const auto& Pair : SkeletonBonePairs )
				{
					SDK::FVector WorldLocationFirst = Custom::GetSocketLocation( Mesh , Pair.first );
					SDK::FVector WorldLocationSecond = Custom::GetSocketLocation( Mesh , Pair.second );

					SDK::FVector2D ScreenPositionFirst = SDK::FVector2D( );
					SDK::FVector2D ScreenPositionSecond = SDK::FVector2D( );

					if ( !Custom::K2_Project( WorldLocationFirst , &ScreenPositionFirst ) ) continue;
					if ( !Custom::K2_Project( WorldLocationSecond , &ScreenPositionSecond ) ) continue;

					Canvas->K2_DrawLine( ScreenPositionFirst , ScreenPositionSecond , 1.5f , SkeletonColor );
				}
			}
		}
	}

	if ( Settings::Aimbot )
	{
		if ( AcknowledgedPawn )
		{
			SDK::APlayerPawn_Athena_C* Target = TargetClosestToCenter;
			if ( Target )
			{
				bool Visable = SDK::FortLibrary->CheckLineOfSightToActorWithChannel( CameraLocation , Target , SDK::ECollisionChannel::ECC_Visibility , AcknowledgedPawn );
				if ( Settings::VisibleOnly && Visable || !Settings::VisibleOnly )
				{
					SDK::USkeletalMeshComponent* Mesh = Target->Mesh( );
					if ( Mesh )
					{
						SDK::FString BoneName = SDK::FString( L"head" );
						if ( Settings::Bone == 1 )
							BoneName = SDK::FString( L"neck_01" );
						else if ( Settings::Bone == 2 )
							BoneName = SDK::FString( L"spine_05" );

						SDK::FVector2D ScreenLocation = SDK::FVector2D( );
						SDK::FVector AimbotPosition = Custom::GetSocketLocation( Mesh , BoneName );
						if ( Custom::K2_Project( AimbotPosition , &ScreenLocation ) )
						{
							SDK::AFortWeapon* CurrentWeapon = AcknowledgedPawn->CurrentWeapon( );
							if ( CurrentWeapon )
							{
								bool IsHoldingWeapon = SDK::GameplayStatics->ObjectIsA( CurrentWeapon , Implementation::Weapon );
								if ( IsHoldingWeapon )
								{
									SDK::FVector StartLocation = CurrentWeapon->GetTargetingSourceLocation( );
									bool IsHoldingShotgun = CurrentWeapon->WeaponCoreAnimation( ) == SDK::EFortWeaponCoreAnimation::Shotgun;
									double PlayerDistance = SDK::MathLibrary->Vector_Distance( StartLocation , AimbotPosition );

									if ( Settings::TargetLine )
									{
										SDK::FVector2D MuzzleScreen = SDK::FVector2D( );
										SDK::FVector Muzzle = CurrentWeapon->GetMuzzleLocation( );
										if ( Custom::K2_Project( Muzzle , &MuzzleScreen ) )
										{
											Canvas->K2_DrawLine( MuzzleScreen , ScreenLocation , 1.f , SDK::FLinearColor( 1.f , 0.f , 0.f , 1.f ) );
										}
									}

									if ( Settings::TriggerBot )
									{
										if ( CurrentWeapon->CanFire( ) && IsHoldingShotgun )
										{
											// No Trigger bot for you
										}
									}

									if ( Settings::KillAll )
									{
										// No Kill All for you
									}

									if ( Settings::AimPrediction )
									{
										SDK::FVector CurrentVelocity = Target->GetVelocity( );
										if ( Target->IsInVehicle( ) || Target->IsDrivingVehicle( ) )
										{
											SDK::AFortAthenaVehicle* CurrentVehicle = Target->CurrentVehicle( );

											CurrentVelocity = CurrentVehicle->GetVelocity( );
										}

										SDK::FVector LocalVelocity = AcknowledgedPawn->GetVelocity( );
										if ( AcknowledgedPawn->IsInVehicle( ) || AcknowledgedPawn->IsDrivingVehicle( ) )
										{
											SDK::AFortAthenaVehicle* CurrentVehicle = AcknowledgedPawn->CurrentVehicle( );

											LocalVelocity = CurrentVehicle->GetVelocity( );
										}

										SDK::FVector RelativeVelocity = CurrentVelocity - LocalVelocity;

										SDK::UFortWeaponItemDefinition* ItemDefinition = CurrentWeapon->WeaponData( );
										bool HasLightweightProjectile = ItemDefinition->HasLightweightProjectile( );
										if ( HasLightweightProjectile )
										{
											// No prediction for you
										}

										if ( Settings::HeadDot )
										{
											SDK::FVector2D PredictionLocation = SDK::FVector2D( );
											if ( Custom::K2_Project( AimbotPosition , &PredictionLocation ) )
											{
												double CircleRaduis = Implementation::ScreenSize.Y / ( 2.0 * PlayerDistance * SDK::MathLibrary->tan( FieldOfView * M_PI / 360.0 ) ) * 8.0;
												Wrapper::Circle( PredictionLocation , SDK::FLinearColor( 1.f , 0.f , 0.f , 1.f ) , CircleRaduis , 138.0 , true );
											}
										}
									}

									if ( Settings::SilentAim )
									{
										Update__GetWeaponTargetingTransform__bSilentAimActive = true;
										Update__GetWeaponTargetingTransform__SilentLocationTarget = AimbotPosition;
									}
									else
									{
										SDK::FKey AimbotKey = Implementation::RightMouseButton;
										if ( Settings::Bind == 1 )
											AimbotKey = Implementation::LeftMouseButton;
										else if ( Settings::Bind == 2 )
											AimbotKey = Implementation::LeftShift;

										bool AimbotKeyDown = PlayerController->IsInputKeyDown( AimbotKey );
										if ( AimbotKeyDown )
										{
											SDK::FRotator AimbotRotation = SDK::MathLibrary->FindLookAtRotation( CameraLocation , AimbotPosition );

											if ( Settings::JitterAmount >= 1.0 )
											{
												Humanization::AddNoise( &AimbotRotation , Settings::JitterAmount );
											}

											if ( Settings::Smoothing >= 1.0 )
											{
												AimbotRotation = SDK::MathLibrary->RInterpTo( CameraRotation , AimbotRotation , WorldDeltaSeconds , Settings::Smoothing );
												AimbotRotation += AimbotRotation - CameraRotation;
											}

											Update__GetControlRotation__bAimbotActive = true;
											Update__GetControlRotation__AimbotRotationTarget = AimbotRotation;
											Update__GetControlRotation__AimbotLocationTarget = AimbotPosition;
										}
									}

									if ( Settings::Crosshair )
									{
										CurrentWeapon->EnableAltCenterReticle( false );
										CurrentWeapon->EnableAltOuterReticle( false );
										CurrentWeapon->EnableCornersReticle( false );

										Canvas->K2_DrawLine( SDK::FVector2D( Implementation::ScreenCenter.X , Implementation::ScreenCenter.Y - 10.0 ) , SDK::FVector2D( Implementation::ScreenCenter.X , Implementation::ScreenCenter.Y + 10.0 ) , 1.f , SDK::FLinearColor( 1.f , 0.f , 0.f , 1.f ) );
										Canvas->K2_DrawLine( SDK::FVector2D( Implementation::ScreenCenter.X - 10.0 , Implementation::ScreenCenter.Y ) , SDK::FVector2D( Implementation::ScreenCenter.X + 10.0 , Implementation::ScreenCenter.Y ) , 1.f , SDK::FLinearColor( 1.f , 0.f , 0.f , 1.f ) );
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ( Settings::WorldESP )
	{
		if ( AcknowledgedPawn )
		{
			SDK::USkeletalMeshComponent* Mesh = AcknowledgedPawn->Mesh( );
			if ( Mesh )
			{
				LocalHeadLocation = Custom::GetSocketLocation( Mesh , SDK::FString( L"head" ) );
			}

			SDK::AFortWeapon* CurrentWeapon = AcknowledgedPawn->CurrentWeapon( );
			if ( CurrentWeapon )
			{
				if ( Settings::NoSpread )
				{
					// No no-spread for you
				}

				if ( Settings::BulletTP )
				{
					SDK::TArray<SDK::UObject*> ProjectileArray = SDK::GameplayStatics->GetAllActorsOfClass( World , Implementation::Projectile );
					for ( int i = 0; i < ProjectileArray.NumElements; i++ )
					{
						if ( !ProjectileArray.IsValid( ) ) continue;
						if ( !ProjectileArray.IsValidIndex( i ) )
							continue;

						SDK::AFortProjectileBase* Projectile = reinterpret_cast < SDK::AFortProjectileBase* >( ProjectileArray[ i ] );
						if ( !Projectile )
							continue;

						SDK::FVector2D ScreenLocation = SDK::FVector2D( );
						SDK::FVector WorldLocation = Projectile->K2_GetActorLocation( );
						if ( !Custom::K2_Project( WorldLocation , &ScreenLocation ) ) continue;
						if ( !Custom::InScreen( ScreenLocation ) ) continue;

						SDK::FVector2D MuzzlePosition = SDK::FVector2D( );
						SDK::FVector MuzzleLocation = CurrentWeapon->GetMuzzleLocation( );
						if ( !Custom::K2_Project( MuzzleLocation , &MuzzlePosition ) ) continue;

						Canvas->K2_DrawLine( MuzzlePosition , ScreenLocation , 1.5f , SDK::FromRGB( 237 , 32 , 118 ) );

						// No projectile TP for you
					}
				}
			}

			if ( Settings::LeProxy )
			{
				// No debug objects for you
			}

			if ( Settings::PickupESP )
			{
				SDK::TArray<SDK::UObject*> PickupArray = SDK::GameplayStatics->GetAllActorsOfClass( World , Implementation::Pickup );
				for ( int i = 0; i < PickupArray.NumElements; i++ )
				{
					if ( !PickupArray.IsValid( ) ) continue;
					if ( !PickupArray.IsValidIndex( i ) )
						continue;

					SDK::AFortPickup* Pickup = reinterpret_cast < SDK::AFortPickup* >( PickupArray[ i ] );
					if ( !Pickup )
						continue;

					if ( !SDK::GameplayStatics->ObjectIsA( Pickup , Implementation::Pickup ) ) continue;

					SDK::FVector2D ScreenLocation = SDK::FVector2D( );
					SDK::FVector WorldLocation = Pickup->K2_GetActorLocation( );
					if ( !Custom::K2_Project( WorldLocation , &ScreenLocation ) ) continue;
					if ( !Custom::InScreen( ScreenLocation ) ) continue;

					double Distance = SDK::MathLibrary->Vector_Distance( LocalHeadLocation , WorldLocation ) * 0.01;
					if ( Distance >= Settings::PickupDistance ) continue;

					SDK::UFortItemDefinition* PickupItemDefinition = Pickup->PrimaryPickupItemEntry( );
					if ( !PickupItemDefinition ) continue;

					SDK::EFortRarity Tier = PickupItemDefinition->Rarity( );
					SDK::FLinearColor Color = Custom::GetColorByTier( Tier );
					SDK::FString DistplayName = PickupItemDefinition->ItemName( ).Get( );
					Canvas->K2_DrawText( Implementation::Font , DistplayName , ScreenLocation , Color , true , false , false );

					SDK::FString ConvertedText = SDK::StringLibrary->BuildString_Double( SDK::FString( ) , SDK::FString( L"(" ) , int( Distance ) , SDK::FString( L")" ) );
					Canvas->K2_DrawText( Implementation::Font , ConvertedText , SDK::FVector2D( ScreenLocation.X , ScreenLocation.Y + LegacyFontSize ) , Color , true , false , false );
				}
			}

			if ( Settings::VehcileESP )
			{
				SDK::TArray<SDK::UObject*> VehicleArray = SDK::GameplayStatics->GetAllActorsOfClass( World , Implementation::Vehicle );
				for ( int i = 0; i < VehicleArray.NumElements; i++ )
				{
					if ( !VehicleArray.IsValid( ) ) continue;
					if ( !VehicleArray.IsValidIndex( i ) )
						continue;

					SDK::AFortAthenaVehicle* Vehicle = reinterpret_cast < SDK::AFortAthenaVehicle* >( VehicleArray[ i ] );
					if ( !Vehicle )
						continue;

					if ( !SDK::GameplayStatics->ObjectIsA( Vehicle , Implementation::Vehicle ) ) continue;

					SDK::FVector2D ScreenLocation = SDK::FVector2D( );
					SDK::FVector WorldLocation = Vehicle->K2_GetActorLocation( );
					if ( !Custom::K2_Project( WorldLocation , &ScreenLocation ) ) continue;
					if ( !Custom::InScreen( ScreenLocation ) ) continue;

					double Distance = SDK::MathLibrary->Vector_Distance( LocalHeadLocation , WorldLocation ) * 0.01;
					if ( Distance >= Settings::PickupDistance ) continue;

					Canvas->K2_DrawText( Implementation::Font , SDK::FString( L"Vehicle" ) , ScreenLocation , SDK::FLinearColor( 0.2f , 0.3f , 0.7f , 1.f ) , true , false , false );

					// No vehicle health esp for you
				}
			}

			if ( Settings::ContainerESP )
			{
				SDK::TArray<SDK::UObject*> ContainerArray = SDK::GameplayStatics->GetAllActorsOfClass( World , Implementation::Container );
				for ( int i = 0; i < ContainerArray.NumElements; i++ )
				{
					if ( !ContainerArray.IsValid( ) ) continue;
					if ( !ContainerArray.IsValidIndex( i ) )
						continue;

					SDK::ABuildingContainer* Container = reinterpret_cast < SDK::ABuildingContainer* >( ContainerArray[ i ] );
					if ( !Container )
						continue;

					if ( !SDK::GameplayStatics->ObjectIsA( Container , Implementation::Container ) ) continue;

					SDK::FVector2D ScreenLocation = SDK::FVector2D( );
					SDK::FVector WorldLocation = Container->K2_GetActorLocation( );
					if ( !Custom::K2_Project( WorldLocation , &ScreenLocation ) ) continue;
					if ( !Custom::InScreen( ScreenLocation ) ) continue;

					double Distance = SDK::MathLibrary->Vector_Distance( LocalHeadLocation , WorldLocation ) * 0.01;
					if ( Distance >= Settings::PickupDistance ) continue;

					if ( Settings::InstantSearch )
					{
						// No instant interaction for you
					}

					SDK::FString ObjectName = SDK::SystemLibrary->GetObjectName( Container );
					if ( SDK::StringLibrary->Contains( ObjectName , SDK::FString( L"Tiered_Chest" ) , true , false ) || SDK::StringLibrary->Contains( ObjectName , SDK::FString( L"BuriedChest" ) , true , false ) )
					{
						Canvas->K2_DrawText( Implementation::Font , SDK::FString( L"Chest" ) , ScreenLocation , SDK::FLinearColor( 1.f , 1.f , 0.f , 1.f ) , true , false , false );

						SDK::FString ConvertedText = SDK::StringLibrary->BuildString_Double( SDK::FString( ) , SDK::FString( L"(" ) , int( Distance ) , SDK::FString( L")" ) );
						Canvas->K2_DrawText( Implementation::Font , ConvertedText , SDK::FVector2D( ScreenLocation.X , ScreenLocation.Y + LegacyFontSize ) , SDK::FLinearColor( 1.f , 1.f , 0.f , 1.f ) , true , false , false );
					}
					else if ( SDK::StringLibrary->Contains( ObjectName , SDK::FString( L"Tiered_Safe" ) , true , false ) )
					{
						Canvas->K2_DrawText( Implementation::Font , SDK::FString( L"Gold Safe" ) , ScreenLocation , SDK::FLinearColor( 1.0f , 0.84f , 0.0f , 1.f ) , true , false , false );

						SDK::FString ConvertedText = SDK::StringLibrary->BuildString_Double( SDK::FString( ) , SDK::FString( L"(" ) , int( Distance ) , SDK::FString( L")" ) );
						Canvas->K2_DrawText( Implementation::Font , ConvertedText , SDK::FVector2D( ScreenLocation.X , ScreenLocation.Y + LegacyFontSize ) , SDK::FLinearColor( 1.0f , 0.84f , 0.0f , 1.f ) , true , false , false );
					}
					else if ( SDK::StringLibrary->Contains( ObjectName , SDK::FString( L"Tiered_Ammo" ) , true , false ) )
					{
						Canvas->K2_DrawText( Implementation::Font , SDK::FString( L"Ammo Box" ) , ScreenLocation , SDK::FLinearColor( 0.5f , 0.5f , 0.5f , 1.f ) , true , false , false );

						SDK::FString ConvertedText = SDK::StringLibrary->BuildString_Double( SDK::FString( ) , SDK::FString( L"(" ) , int( Distance ) , SDK::FString( L")" ) );
						Canvas->K2_DrawText( Implementation::Font , ConvertedText , SDK::FVector2D( ScreenLocation.X , ScreenLocation.Y + LegacyFontSize ) , SDK::FLinearColor( 0.5f , 0.5f , 0.5f , 1.f ) , true , false , false );
					}
				}
			}

			if ( Settings::SupplyDropESP )
			{
				SDK::TArray<SDK::UObject*> SupplyDropArray = SDK::GameplayStatics->GetAllActorsOfClass( World , Implementation::SupplyDrop );
				for ( int i = 0; i < SupplyDropArray.NumElements; i++ )
				{
					if ( !SupplyDropArray.IsValid( ) ) continue;
					if ( !SupplyDropArray.IsValidIndex( i ) )
						continue;

					SDK::AFortAthenaSupplyDrop* SupplyDrop = reinterpret_cast < SDK::AFortAthenaSupplyDrop* >( SupplyDropArray[ i ] );
					if ( !SupplyDrop )
						continue;

					if ( !SDK::GameplayStatics->ObjectIsA( SupplyDrop , Implementation::SupplyDrop ) ) continue;

					SDK::FVector2D ScreenLocation = SDK::FVector2D( );
					SDK::FVector WorldLocation = SupplyDrop->K2_GetActorLocation( );
					if ( !Custom::K2_Project( WorldLocation , &ScreenLocation ) ) continue;
					if ( !Custom::InScreen( ScreenLocation ) ) continue;

					double Distance = SDK::MathLibrary->Vector_Distance( LocalHeadLocation , WorldLocation ) * 0.01;
					if ( Distance >= Settings::PickupDistance ) continue;

					SDK::FString ObjectName = SDK::SystemLibrary->GetObjectName( SupplyDrop );
					if ( SDK::StringLibrary->Contains( ObjectName , SDK::FString( L"AthenaSupplyDrop_Llama" ) , true , false ) )
					{
						Canvas->K2_DrawText( Implementation::Font , SDK::FString( L"Llama" ) , ScreenLocation , SDK::FLinearColor( 0.5f , 0.0f , 1.f , 1.f ) , true , false , false );

						SDK::FString ConvertedText = SDK::StringLibrary->BuildString_Double( SDK::FString( ) , SDK::FString( L"(" ) , int( Distance ) , SDK::FString( L")" ) );
						Canvas->K2_DrawText( Implementation::Font , ConvertedText , SDK::FVector2D( ScreenLocation.X , ScreenLocation.Y + LegacyFontSize ) , SDK::FLinearColor( 0.5f , 0.0f , 1.f , 1.f ) , true , false , false );
					}
					else if ( SDK::StringLibrary->Contains( ObjectName , SDK::FString( L"AthenaSupplyDrop_C" ) , true , false ) )
					{
						Canvas->K2_DrawText( Implementation::Font , SDK::FString( L"SupplyDrop" ) , ScreenLocation , SDK::FLinearColor( 0.1f , 0.5f , 0.9f , 1.0f ) , true , false , false );

						SDK::FString ConvertedText = SDK::StringLibrary->BuildString_Double( SDK::FString( ) , SDK::FString( L"(" ) , int( Distance ) , SDK::FString( L")" ) );
						Canvas->K2_DrawText( Implementation::Font , ConvertedText , SDK::FVector2D( ScreenLocation.X , ScreenLocation.Y + LegacyFontSize ) , SDK::FLinearColor( 0.1f , 0.5f , 0.9f , 1.0f ) , true , false , false );
					}
				}
			}

			if ( Settings::WeakspotAimbot )
			{
				double ClosestDistanceToCenter = DBL_MAX;
				SDK::ABuildingWeakSpot* TargetWeakSpot = nullptr;

				SDK::TArray<SDK::UObject*> WeakspotArray = SDK::GameplayStatics->GetAllActorsOfClass( World , Implementation::Weakspot );
				for ( int i = 0; i < WeakspotArray.NumElements; i++ )
				{
					if ( !WeakspotArray.IsValid( ) ) continue;
					if ( !WeakspotArray.IsValidIndex( i ) )
						continue;

					SDK::ABuildingWeakSpot* Weakspot = reinterpret_cast < SDK::ABuildingWeakSpot* >( WeakspotArray[ i ] );
					if ( !Weakspot )
						continue;

					if ( !SDK::GameplayStatics->ObjectIsA( Weakspot , Implementation::Weakspot ) ) continue;

					SDK::FVector2D ScreenLocation = SDK::FVector2D( );
					SDK::FVector WorldLocation = Weakspot->K2_GetActorLocation( );
					if ( !Custom::K2_Project( WorldLocation , &ScreenLocation ) ) continue;
					if ( !Custom::InScreen( ScreenLocation ) ) continue;

					double Distance = SDK::MathLibrary->Vector_Distance( LocalHeadLocation , WorldLocation ) * 0.01;
					if ( Distance >= 25.0 ) continue;

					if ( Weakspot->bHit( ) ) continue;
					if ( !Weakspot->bActive( ) ) continue;

					Canvas->K2_DrawText( Implementation::Font , SDK::FString( L"WS" ) , ScreenLocation , SDK::FLinearColor( 1.f , 0.f , 0.f , 1.f ) , true , false , false );

					SDK::FString ConvertedText = SDK::StringLibrary->BuildString_Double( SDK::FString( ) , SDK::FString( L"(" ) , int( Distance ) , SDK::FString( L")" ) );
					Canvas->K2_DrawText( Implementation::Font , ConvertedText , SDK::FVector2D( ScreenLocation.X , ScreenLocation.Y + LegacyFontSize ) , SDK::FLinearColor( 1.f , 0.f , 0.f , 1.f ) , true , false , false );

					double DistanceToCenter = SDK::MathLibrary->Vector_Distance2D( Implementation::ScreenCenter , ScreenLocation );
					if ( DistanceToCenter < ClosestDistanceToCenter )
					{
						if ( Custom::InCircle( AimbotFOV , ScreenLocation ) )
						{
							ClosestDistanceToCenter = DistanceToCenter;
							TargetWeakSpot = Weakspot;
						}
					}
				}

				if ( TargetWeakSpot )
				{
					SDK::AFortWeapon* CurrentWeapon = AcknowledgedPawn->CurrentWeapon( );
					bool IsHoldingPickaxe = SDK::GameplayStatics->ObjectIsA( CurrentWeapon , Implementation::Pickaxe );
					if ( IsHoldingPickaxe )
					{
						if ( SDK::GameplayStatics->ObjectIsA( CurrentWeapon , Implementation::Pickaxe ) )
						{
							SDK::FVector WorldLocation = TargetWeakSpot->K2_GetActorLocation( );
							SDK::FVector2D ScreenLocation = SDK::FVector2D( );
							if ( Custom::K2_Project( WorldLocation , &ScreenLocation ) )
							{
								Canvas->K2_DrawLine( ScreenLocation , Implementation::ScreenCenter , 1.f , SDK::FLinearColor( 1.0f , 0.65f , 0.0f , 1.0f ) );

								SDK::FKey AimbotKey = Implementation::RightMouseButton;
								if ( Settings::Bind == 1 )
									AimbotKey = Implementation::LeftMouseButton;
								else if ( Settings::Bind == 2 )
									AimbotKey = Implementation::LeftShift;

								bool AimbotKeyDown = PlayerController->IsInputKeyDown( AimbotKey );
								if ( AimbotKeyDown )
								{
									SDK::FRotator TargetRotation = SDK::MathLibrary->FindLookAtRotation( CameraLocation , WorldLocation );

									if ( Settings::Smooth >= 0.0 )
									{
										TargetRotation = SDK::MathLibrary->RInterpTo( CameraRotation , TargetRotation , WorldDeltaSeconds , Settings::Smoothing );
										if ( Settings::Smooth == 1 )
											TargetRotation = SDK::MathLibrary->RInterpTo_Constant( CameraRotation , TargetRotation , WorldDeltaSeconds , Settings::Smoothing );

										TargetRotation += ( TargetRotation - CameraRotation );
									}

									Update__GetControlRotation__bAimbotActive = true;
									Update__GetControlRotation__AimbotRotationTarget = TargetRotation;
								}
							}
						}
					}
				}
			}
		}

		if ( Settings::Building )
		{
			SDK::TArray<SDK::UObject*> BuildingTrapArray = SDK::GameplayStatics->GetAllActorsOfClass( World , Implementation::Building );
			for ( int i = 0; i < BuildingTrapArray.NumElements; i++ )
			{
				if ( !BuildingTrapArray.IsValid( ) ) continue;
				if ( !BuildingTrapArray.IsValidIndex( i ) )
					continue;

				SDK::ABuildingTrap* BuildingTrap = reinterpret_cast < SDK::ABuildingTrap* >( BuildingTrapArray[ i ] );
				if ( !BuildingTrap )
					continue;

				if ( !SDK::GameplayStatics->ObjectIsA( BuildingTrap , Implementation::Building ) ) continue;

				SDK::FVector2D ScreenLocation = SDK::FVector2D( );
				SDK::FVector WorldLocation = BuildingTrap->K2_GetActorLocation( );
				if ( !Custom::K2_Project( WorldLocation , &ScreenLocation ) ) continue;
				if ( !Custom::InScreen( ScreenLocation ) ) continue;

				double Distance = SDK::MathLibrary->Vector_Distance( LocalHeadLocation , WorldLocation ) * 0.01;
				if ( Distance >= Settings::PickupDistance ) continue;

				Canvas->K2_DrawText( Implementation::Font , SDK::FString( L"Building Trap" ) , ScreenLocation , SDK::FLinearColor( 0.f , 0.5f , 1.f , 1.f ) , true , false , false );

				// No health ESP for you

				SDK::FString ConvertedText = SDK::StringLibrary->BuildString_Double( SDK::FString( ) , SDK::FString( L"(" ) , int( Distance ) , SDK::FString( L")" ) );
				Canvas->K2_DrawText( Implementation::Font , ConvertedText , SDK::FVector2D( ScreenLocation.X , ScreenLocation.Y + ( LegacyFontSize + LegacyFontSize ) ) , SDK::FLinearColor( 0.f , 1.f , 0.f , 1.f ) , true , false , false );
			}

			SDK::TArray<SDK::ABuildingActor*> HighlightedPrimaryBuildings = PlayerController->HighlightedPrimaryBuildings( );
			for ( int32_t i = 0; i < HighlightedPrimaryBuildings.NumElements; i++ )
			{
				if ( !HighlightedPrimaryBuildings.IsValid( ) ) continue;
				if ( !HighlightedPrimaryBuildings.IsValidIndex( i ) )
					continue;

				SDK::ABuildingActor* HighlightedPrimaryBuilding = HighlightedPrimaryBuildings[ i ];
				if ( !HighlightedPrimaryBuilding ) continue;

				SDK::FVector2D ScreenLocation = SDK::FVector2D( );
				SDK::FVector WorldLocation = HighlightedPrimaryBuilding->K2_GetActorLocation( );
				if ( !Custom::K2_Project( WorldLocation , &ScreenLocation ) ) continue;
				if ( !Custom::InScreen( ScreenLocation ) ) continue;

				bool HasHealthLeft = HighlightedPrimaryBuilding->HasHealthLeft( );
				if ( !HasHealthLeft ) continue;

				HighlightedPrimaryBuilding->SetQuestHighlight( true );
				HighlightedPrimaryBuilding->SetSuppressHealthBar( true );

				// No health ESP for you
			}
		}
	}

	static bool IsMenuOpen = true;
	if ( PlayerController->WasInputKeyJustPressed( Implementation::Insert ) ) IsMenuOpen = !IsMenuOpen;
	static Framework::Render GUI;

	if ( IsMenuOpen )
	{
		GUI.Setup( Canvas , PlayerController , Implementation::Font );

		GUI.Begin( SDK::FString( L"BackBone Fortnite Internal Lite" ) );

		static int MenuTab = 0;
		if ( GUI.Button( SDK::FString( L"Aimbot" ) , MenuTab == 0 ) ) MenuTab = 0;
		if ( GUI.Button( SDK::FString( L"Visuals" ) , MenuTab == 1 ) ) MenuTab = 1;
		if ( GUI.Button( SDK::FString( L"Environment" ) , MenuTab == 2 ) ) MenuTab = 2;
		if ( GUI.Button( SDK::FString( L"Skin Changer" ) , MenuTab == 3 ) ) MenuTab = 3;
		if ( GUI.Button( SDK::FString( L"Exploits" ) , MenuTab == 4 ) )  MenuTab = 4;
		if ( GUI.Button( SDK::FString( L"Misc" ) , MenuTab == 5 ) )  MenuTab = 5;

		GUI.ResetY( );

		if ( MenuTab == 0 )
		{
			GUI.Checkbox( SDK::FString( L"Memory Aim" ) , &Settings::Aimbot );
			GUI.Checkbox( SDK::FString( L"FOV Circle" ) , &Settings::FOVCircle );
			GUI.Checkbox( SDK::FString( L"Target Line" ) , &Settings::TargetLine );
			GUI.Checkbox( SDK::FString( L"Ignore Bots" ) , &Settings::IgnoreBots );
			GUI.Checkbox( SDK::FString( L"Ignore Non-Visible" ) , &Settings::VisibleOnly );
			GUI.Checkbox( SDK::FString( L"Engine Prediction" ) , &Settings::AimPrediction );
			GUI.Slider( SDK::FString( L"FieldOfView" ) , 0.0 , 80.0 , &Settings::FieldOfView );
			GUI.Slider( SDK::FString( L"Speed" ) , 0.0 , 20.0 , &Settings::Smoothing );

			static bool ActiveHitbone = false;
			SDK::FString Hitbones[ ] = { SDK::FString( L"Head" ), SDK::FString( L"Neck" ), SDK::FString( L"Chest" ) };
			GUI.Combobox( SDK::FString( L"Aim Bone" ) , &Settings::Bone , Hitbones , sizeof( Hitbones ) / sizeof( SDK::FString ), &ActiveHitbone );

			static bool ActiveToggleKey = false;
			SDK::FString ToggleKeys[ ] = { SDK::FString( L"RMB" ), SDK::FString( L"LMB" ), SDK::FString( L"Shift" ) };
			GUI.Combobox( SDK::FString( L"Toggle Key" ) , &Settings::Bind , ToggleKeys , sizeof( ToggleKeys ) / sizeof( SDK::FString ), &ActiveToggleKey );

			GUI.SwitchForm( );

			GUI.Checkbox( SDK::FString( L"Trigger Bot" ) , &Settings::TriggerBot );
			GUI.Checkbox( SDK::FString( L"ShotGun Only" ) , &Settings::OnlyShotgun );
			GUI.Checkbox( SDK::FString( L"Weakspot Aimbot" ) , &Settings::WeakspotAimbot );
			GUI.Slider( SDK::FString( L"Aimbot Distance" ) , 0.0 , 500.0 , &Settings::AimbotDistance );

			GUI.ResetForm( );
		}
		else if ( MenuTab == 1 )
		{
			GUI.Checkbox( SDK::FString( L"Enable" ) , &Settings::Visuals );
			GUI.Checkbox( SDK::FString( L"Name ESP" ) , &Settings::Username );
			GUI.Checkbox( SDK::FString( L"Line ESP" ) , &Settings::Line );
			GUI.Checkbox( SDK::FString( L"Radar ESP" ) , &Settings::Radar );
			GUI.Checkbox( SDK::FString( L"Filled Box" ) , &Settings::Fill );
			GUI.Checkbox( SDK::FString( L"Head Dot ESP" ) , &Settings::HeadDot );
			GUI.Checkbox( SDK::FString( L"Skeleton ESP" ) , &Settings::Skeleton );
			GUI.Checkbox( SDK::FString( L"Distance ESP" ) , &Settings::Distance );
			GUI.Checkbox( SDK::FString( L"Active Weapon" ) , &Settings::ActiveWeapon );

			static bool ActiveBoxType = false;
			SDK::FString BoxType[ ] = { SDK::FString( L"Disabled" ), SDK::FString( L"2D" ), SDK::FString( L"Corner" ) };
			GUI.Combobox( SDK::FString( L"Box ESP" ) , &Settings::Boxes , BoxType , sizeof( BoxType ) / sizeof( SDK::FString ), &ActiveBoxType );
		}
		else if ( MenuTab == 2 )
		{
			GUI.Checkbox( SDK::FString( L"Enable" ) , &Settings::WorldESP );
			GUI.Checkbox( SDK::FString( L"Pickup ESP" ) , &Settings::PickupESP );
			GUI.Checkbox( SDK::FString( L"Vehicle ESP" ) , &Settings::VehcileESP );
			GUI.Checkbox( SDK::FString( L"Building ESP" ) , &Settings::Building );
			GUI.Checkbox( SDK::FString( L"Debug Objects" ) , &Settings::LeProxy );
			GUI.Checkbox( SDK::FString( L"Chest/AmmoBox ESP" ) , &Settings::ContainerESP );
			GUI.Checkbox( SDK::FString( L"Llama/SupplyDrop ESP" ) , &Settings::SupplyDropESP );

			GUI.SwitchForm( );

			GUI.Slider( SDK::FString( L"Distance Limit" ) , 0.0 , 500.0 , &Settings::PickupDistance );

			GUI.ResetForm( );
		}
		else if ( MenuTab == 4 )
		{
			GUI.Checkbox( SDK::FString( L"Silent Aimbot" ) , &Settings::SilentAim );
			GUI.Checkbox( SDK::FString( L"No Weapon Spread" ) , &Settings::NoSpread );
			GUI.Checkbox( SDK::FString( L"Instant Kill All" ) , &Settings::KillAll );
			GUI.Checkbox( SDK::FString( L"Projectile Teleport" ) , &Settings::BulletTP );
			GUI.Checkbox( SDK::FString( L"Instant Interaction" ) , &Settings::InstantSearch );

			GUI.SwitchForm( );

			GUI.Checkbox( SDK::FString( L"Player Chams" ) , &Settings::Chams );
			GUI.Checkbox( SDK::FString( L"Wireframe Chams" ) , &Settings::Wireframe );
			GUI.Checkbox( SDK::FString( L"Chams Through Walls" ) , &Settings::Wallhacks );

			GUI.ResetForm( );
		}

		GUI.EndRender( );
	}

	Implementation::bAimbotActive = Update__GetControlRotation__bAimbotActive;
	Implementation::AimbotRotationTarget = Update__GetControlRotation__AimbotRotationTarget;

	Implementation::bSilentAimActive = Update__GetWeaponTargetingTransform__bSilentAimActive;
	Implementation::SilentAimLocationTarget = Update__GetWeaponTargetingTransform__SilentLocationTarget;

	return DrawTransitionOriginal( ViewportClient , Canvas );
}
