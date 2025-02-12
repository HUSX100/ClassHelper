using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using ClassIsland.Core.Abstractions.Services;
using ClassIsland.Shared.Interfaces;
using ClassIsland.Shared.Models.Notification;
using MaterialDesignThemes.Wpf;
using Microsoft.Extensions.Hosting;
using Plugin_RollCaller.Controls.NotificationProviders;
using Plugin_RollCaller.Models;

namespace Plugin_RollCaller.Services.NotificationProviders;

public class MyNotificationProvider : INotificationProvider, IHostedService
{
    private INotificationHostService NotificationHostService { get; }
    public IUriNavigationService UriNavigationService { get; }
    public string Name { get; set; } = "RollCallerServices";
    public string Description { get; set; } = "用于为点名器提供通知接口";
    public Guid ProviderGuid { get; set; } = new Guid("DD3BC389-BEA9-40B7-912B-C7C37390A101");
    public object? SettingsElement { get; set; }
    public object? IconElement { get; set; } =  new PackIcon()
    {
        Kind = PackIconKind.AccountAlert,
        Width = 24,
        Height = 24
    };
    
    /// <summary>
    /// 这个属性用来存储提醒的设置。
    /// </summary>
    private MyNotificationSettings Settings { get; }

    public MyNotificationProvider(INotificationHostService notificationHostService, IUriNavigationService uriNavigationService)
    {
        NotificationHostService = notificationHostService;
        UriNavigationService = uriNavigationService;
        NotificationHostService.RegisterNotificationProvider(this);
        
        // 获取这个提醒提供方的设置，并保存到 Settings 属性上备用。
        Settings = NotificationHostService.GetNotificationProviderSettings<MyNotificationSettings>(ProviderGuid);

        // 将刚刚获取到的提醒提供方设置传给提醒设置控件，这样提醒设置控件就可以访问到提醒设置了。
        // 然后将 SettingsElement 属性设置为这个控件对象，这样提醒设置界面就会显示我们自定义的提醒设置控件。
        SettingsElement = new MyNotificationProviderSettingsControl(Settings);

        UriNavigationService.HandlePluginsNavigation(
            "RollCaller/Run",
            args => {
                UriOn(null, EventArgs.Empty);
            }
        );
    }

    [DllImport(".\\ClassHelper\\core.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr GetRandomStudentName();

    private void UriOn(object? sender, EventArgs e)
    {
        IntPtr ptr = GetRandomStudentName();
        string output = Marshal.PtrToStringBSTR(ptr);
        Marshal.FreeBSTR(ptr); // 释放分配的 BSTR 内存

        NotificationHostService.ShowNotification(new NotificationRequest()
        {
            MaskContent = new TextBlock(new Run(output))
            {
                VerticalAlignment = VerticalAlignment.Center,
                HorizontalAlignment = HorizontalAlignment.Center
            },
            MaskDuration = new TimeSpan(0, 0, 3),
        });
    }

    public async Task StartAsync(CancellationToken cancellationToken)
    {
    }

    public async Task StopAsync(CancellationToken cancellationToken)
    {
    }
}